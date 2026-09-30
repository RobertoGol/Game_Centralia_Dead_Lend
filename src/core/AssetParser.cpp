#include "video/AssetParser.hpp"
#include "platform/Platform.hpp"
#include "core/MemoryManager.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <filesystem>
#include <unordered_map>

// Кроссплатформенный Memory Mapping для Zero-Copy I/O
#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

// Сторонняя библиотека декомпрессии (эмуляция вызовов zlib/lz4)
// #include <zlib.h>
// #include <lz4.h>

namespace fs = std::filesystem;

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS, MAGIC NUMBERS & CA2 ARCHIVE HEADERS
// ============================================================================

namespace AssetConfig {
    constexpr uint32_t CA2_MAGIC = 0x324143; // "CA2\0" (Centralia Archive 2)
    constexpr uint32_t CA2_VERSION = 104;
    constexpr uint32_t MAX_ASYNC_WORKERS = 4;
}

// Заголовок архива (аналог Bethesda Archive 2)
#pragma pack(push, 1)
struct CA2Header {
    uint32_t magic;           // Идентификатор формата
    uint32_t version;         // Версия архива
    uint32_t directoryOffset; // Смещение до таблицы файлов
    uint32_t fileCount;       // Количество файлов в архиве
    uint32_t nameTableOffset; // Смещение до таблицы строковых имен
    uint64_t totalArchiveSize;// Общий размер файла
};

// Запись о файле внутри архива CA2
struct CA2FileEntry {
    uint32_t nameHash;        // Хэш имени файла (MurmurHash3)
    uint32_t extHash;         // Хэш расширения (например, .dds, .mesh)
    uint32_t directoryIndex;  // Индекс в структуре директорий
    uint32_t attributes;      // Флаги (сжат, зашифрован, потоковый)
    uint64_t offset;          // Абсолютное смещение от начала архива
    uint32_t packedSize;      // Размер сжатых данных (LZ4/Zlib)
    uint32_t unpackedSize;    // Исходный размер файла
};
#pragma pack(pop)

// ============================================================================
// SECTION 2: FAST HASHING (MURMURHASH3) FOR PATH RESOLUTION
// ============================================================================

namespace HashUtils {
    static inline uint32_t ROTL32(uint32_t x, int8_t r) {
        return (x << r) | (x >> (32 - r));
    }

    uint32_t MurmurHash3_x86_32(const void* key, int len, uint32_t seed = 0x9E3779B1) {
        const uint8_t* data = (const uint8_t*)key;
        const int nblocks = len / 4;
        uint32_t h1 = seed;
        const uint32_t c1 = 0xcc9e2d51;
        const uint32_t c2 = 0x1b873593;

        const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);
        for(int i = -nblocks; i; i++) {
            uint32_t k1 = blocks[i];
            k1 *= c1; k1 = ROTL32(k1, 15); k1 *= c2;
            h1 ^= k1; h1 = ROTL32(h1, 13); h1 = h1 * 5 + 0xe6546b64;
        }

        const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);
        uint32_t k1 = 0;
        switch(len & 3) {
            case 3: k1 ^= tail[2] << 16;
            case 2: k1 ^= tail[1] << 8;
            case 1: k1 ^= tail[0];
                    k1 *= c1; k1 = ROTL32(k1, 15); k1 *= c2; h1 ^= k1;
        };

        h1 ^= len;
        h1 ^= h1 >> 16; h1 *= 0x85ebca6b;
        h1 ^= h1 >> 13; h1 *= 0xc2b2ae35; h1 ^= h1 >> 16;
        return h1;
    }

    uint32_t HashFilePath(const std::string& path) {
        std::string lowerPath = path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
        // Заменяем все обратные слеши на прямые для унификации
        std::replace(lowerPath.begin(), lowerPath.end(), '\\', '/');
        return MurmurHash3_x86_32(lowerPath.c_str(), static_cast<int>(lowerPath.length()));
    }
}

// ============================================================================
// SECTION 3: CROSS-PLATFORM MEMORY MAPPED FILE SYSTEM (ZERO-COPY)
// ============================================================================

class MemoryMappedArchive {
private:
    std::string m_filePath;
    void* m_mappedData;
    size_t m_fileSize;

#if defined(_WIN32) || defined(_WIN64)
    HANDLE m_hFile;
    HANDLE m_hMapping;
#else
    int m_fd;
#endif

public:
    MemoryMappedArchive() : m_mappedData(nullptr), m_fileSize(0) {
#if defined(_WIN32) || defined(_WIN64)
        m_hFile = INVALID_HANDLE_VALUE;
        m_hMapping = NULL;
#else
        m_fd = -1;
#endif
    }

    ~MemoryMappedArchive() { Close(); }

    bool Open(const std::string& path) {
        m_filePath = path;
#if defined(_WIN32) || defined(_WIN64)
        m_hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (m_hFile == INVALID_HANDLE_VALUE) return false;

        LARGE_INTEGER size;
        GetFileSizeEx(m_hFile, &size);
        m_fileSize = size.QuadPart;

        m_hMapping = CreateFileMappingA(m_hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (m_hMapping == NULL) { Close(); return false; }

        m_mappedData = MapViewOfFile(m_hMapping, FILE_MAP_READ, 0, 0, 0);
        if (m_mappedData == nullptr) { Close(); return false; }
#else
        m_fd = open(path.c_str(), O_RDONLY);
        if (m_fd == -1) return false;

        struct stat sb;
        if (fstat(m_fd, &sb) == -1) { Close(); return false; }
        m_fileSize = sb.st_size;

        m_mappedData = mmap(NULL, m_fileSize, PROT_READ, MAP_PRIVATE, m_fd, 0);
        if (m_mappedData == MAP_FAILED) { m_mappedData = nullptr; Close(); return false; }
#endif
        return true;
    }

    void Close() {
#if defined(_WIN32) || defined(_WIN64)
        if (m_mappedData) UnmapViewOfFile(m_mappedData);
        if (m_hMapping) CloseHandle(m_hMapping);
        if (m_hFile != INVALID_HANDLE_VALUE) CloseHandle(m_hFile);
        m_mappedData = nullptr; m_hMapping = NULL; m_hFile = INVALID_HANDLE_VALUE;
#else
        if (m_mappedData) munmap(m_mappedData, m_fileSize);
        if (m_fd != -1) close(m_fd);
        m_mappedData = nullptr; m_fd = -1;
#endif
        m_fileSize = 0;
    }

    const uint8_t* GetData() const { return static_cast<const uint8_t*>(m_mappedData); }
    size_t GetSize() const { return m_fileSize; }
};

// ============================================================================
// SECTION 4: ASYNC JOB SYSTEM FOR BACKGROUND ASSET LOADING
// ============================================================================

struct AssetLoadJob {
    std::string virtualPath;
    std::function<void(const std::vector<uint8_t>&)> onComplete;
};

class AsyncAssetLoader {
private:
    std::vector<std::thread> m_workers;
    std::queue<AssetLoadJob> m_jobQueue;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    bool m_terminate;
    AssetParser* m_parserInstance;

    void WorkerThread() {
        while (true) {
            AssetLoadJob job;
            {
                std::unique_lock<std::mutex> lock(m_queueMutex);
                m_condition.wait(lock, [this]() { return m_terminate || !m_jobQueue.empty(); });
                if (m_terminate && m_jobQueue.empty()) return;

                job = std::move(m_jobQueue.front());
                m_jobQueue.pop();
            }

            // Выполнение тяжелой задачи загрузки и парсинга
            std::vector<uint8_t> data = m_parserInstance->LoadRawDataSync(job.virtualPath);
            
            // Вызов коллбэка по завершении (передача собранных данных)
            if (job.onComplete && !data.empty()) {
                job.onComplete(data);
            }
        }
    }

public:
    AsyncAssetLoader(AssetParser* parser) : m_terminate(false), m_parserInstance(parser) {
        for (uint32_t i = 0; i < AssetConfig::MAX_ASYNC_WORKERS; ++i) {
            m_workers.emplace_back(&AsyncAssetLoader::WorkerThread, this);
        }
        Platform::Log("[ASSET WORKERS]: Запущено " + std::to_string(AssetConfig::MAX_ASYNC_WORKERS) + " фоновых потоков загрузки.");
    }

    ~AsyncAssetLoader() {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_terminate = true;
        }
        m_condition.notify_all();
        for (auto& worker : m_workers) {
            if (worker.joinable()) worker.join();
        }
    }

    void QueueJob(const std::string& path, std::function<void(const std::vector<uint8_t>&)> callback) {
        {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_jobQueue.push({path, callback});
        }
        m_condition.notify_one();
    }
};

// ============================================================================
// SECTION 5: ASSET PARSER INTERNAL STATE & MOUNTING LOGIC
// ============================================================================

struct MountedArchive {
    std::string archiveName;
    MemoryMappedArchive mappedFile;
    std::unordered_map<uint32_t, CA2FileEntry> fileLookupTable; // PathHash -> Entry
};

struct AssetParserImpl {
    std::vector<std::unique_ptr<MountedArchive>> mountedArchives;
    std::string looseFilesRootPath; // Директория с неупакованными файлами (Data/)
    std::unique_ptr<AsyncAssetLoader> asyncLoader;
    std::mutex parserMutex;
};

AssetParser* AssetParser::s_instance = nullptr;

AssetParser::AssetParser() : m_pImpl(new AssetParserImpl()) {
    if (s_instance) std::terminate();
    s_instance = this;

    m_pImpl->looseFilesRootPath = "Data/"; // Папка для Loose-файлов по умолчанию
    m_pImpl->asyncLoader = std::make_unique<AsyncAssetLoader>(this);

    Platform::Log("[ASSET PARSER]: Виртуальная файловая система ресурсов инициализирована.");
}

AssetParser::~AssetParser() {
    m_pImpl->asyncLoader.reset();
    m_pImpl->mountedArchives.clear();
    delete m_pImpl;
    s_instance = nullptr;
    Platform::Log("[ASSET PARSER]: VFS отключена. Память архивов освобождена (Unmapped).");
}

AssetParser& AssetParser::GetInstance() {
    if (!s_instance) std::terminate();
    return *s_instance;
}

void AssetParser::SetLooseFilesDirectory(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_pImpl->parserMutex);
    m_pImpl->looseFilesRootPath = path;
    if (m_pImpl->looseFilesRootPath.back() != '/' && m_pImpl->looseFilesRootPath.back() != '\\') {
        m_pImpl->looseFilesRootPath += "/";
    }
    Platform::Log("[ASSET PARSER]: Установлена корневая директория для свободных файлов: " + m_pImpl->looseFilesRootPath);
}

bool AssetParser::MountArchive(const std::string& archivePath) {
    std::lock_guard<std::mutex> lock(m_pImpl->parserMutex);

    auto archive = std::make_unique<MountedArchive>();
    archive->archiveName = archivePath;

    if (!archive->mappedFile.Open(archivePath)) {
        Platform::Log("[ASSET ERROR]: Не удалось открыть архив для отображения в память: " + archivePath);
        return false;
    }

    const uint8_t* data = archive->mappedFile.GetData();
    size_t size = archive->mappedFile.GetSize();

    if (size < sizeof(CA2Header)) {
        Platform::Log("[ASSET ERROR]: Поврежденный заголовок архива: " + archivePath);
        return false;
    }

    const CA2Header* header = reinterpret_cast<const CA2Header*>(data);
    
    if (header->magic != AssetConfig::CA2_MAGIC) {
        Platform::Log("[ASSET ERROR]: Неверный Magic Identifier архива. Ожидалось 'CA2'.");
        return false;
    }

    // Парсинг таблицы файлов (File Index Table)
    const CA2FileEntry* fileEntries = reinterpret_cast<const CA2FileEntry*>(data + header->directoryOffset);
    
    for (uint32_t i = 0; i < header->fileCount; ++i) {
        archive->fileLookupTable[fileEntries[i].nameHash] = fileEntries[i];
    }

    Platform::Log("[ASSET MOUNT]: Архив '" + archivePath + "' успешно смонтирован. Индексировано файлов: " + std::to_string(header->fileCount));
    m_pImpl->mountedArchives.push_back(std::move(archive));
    return true;
}

// ============================================================================
// SECTION 6: RAW DATA RESOLUTION (LOOSE FILES VS ARCHIVES)
// ============================================================================

std::vector<uint8_t> AssetParser::LoadRawDataSync(const std::string& virtualPath) {
    // 1. ПРИОРИТЕТ LOOSE FILES (Моддинг / Сортировка)
    // Если файл лежит физически в структуре директорий - загружаем его, игнорируя архив.
    std::string physicalPath = m_pImpl->looseFilesRootPath + virtualPath;
    if (fs::exists(physicalPath) && !fs::is_directory(physicalPath)) {
        std::ifstream file(physicalPath, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            size_t size = file.tellg();
            std::vector<uint8_t> buffer(size);
            file.seekg(0, std::ios::beg);
            file.read(reinterpret_cast<char*>(buffer.data()), size);
            file.close();
            // Platform::Log("[ASSET FETCH]: Свободный файл (Loose) переопределил ресурсы архива: " + virtualPath);
            return buffer;
        }
    }

    // 2. ПОИСК В ЗАПАКАОВАННЫХ АРХИВАХ (CA2)
    uint32_t pathHash = HashUtils::HashFilePath(virtualPath);
    
    std::lock_guard<std::mutex> lock(m_pImpl->parserMutex);

    // Ищем в обратном порядке монтирования (последний смонтированный архив переопределяет предыдущие, как в BA2 патчах)
    for (auto it = m_pImpl->mountedArchives.rbegin(); it != m_pImpl->mountedArchives.rend(); ++it) {
        MountedArchive* archive = it->get();
        auto fileIt = archive->fileLookupTable.find(pathHash);
        
        if (fileIt != archive->fileLookupTable.end()) {
            const CA2FileEntry& entry = fileIt->second;
            const uint8_t* rawData = archive->mappedFile.GetData() + entry.offset;

            // Если файл сжат (флаг 0x01)
            if (entry.attributes & 0x01) {
                // В реальном движке здесь вызывается декомпрессор LZ4 / Zlib
                // std::vector<uint8_t> decompressed(entry.unpackedSize);
                // LZ4_decompress_safe((const char*)rawData, (char*)decompressed.data(), entry.packedSize, entry.unpackedSize);
                // return decompressed;
                
                Platform::Log("[ASSET ERROR]: Запрошен сжатый файл, но декомпрессор не слинкован: " + virtualPath);
                return std::vector<uint8_t>();
            } else {
                // Прямое копирование несжатого чанка из Memory Map
                return std::vector<uint8_t>(rawData, rawData + entry.unpackedSize);
            }
        }
    }

    Platform::Log("[ASSET ERROR]: Файл не найден ни в директориях, ни в архивах CA2: " + virtualPath);
    return std::vector<uint8_t>();
}

void AssetParser::LoadRawDataAsync(const std::string& virtualPath, std::function<void(const std::vector<uint8_t>&)> callback) {
    m_pImpl->asyncLoader->QueueJob(virtualPath, callback);
}

// ============================================================================
// SECTION 7: SPECIFIC PARSERS - DIRECTDRAW SURFACE (DDS TEXTURES)
// ============================================================================

#pragma pack(push, 1)
struct DDS_PIXELFORMAT {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
};

struct DDS_HEADER {
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t dwCaps;
    uint32_t dwCaps2;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};
#pragma pack(pop)

constexpr uint32_t DDS_MAGIC = 0x20534444; // "DDS "

ParsedTexture AssetParser::ParseDDS(const std::vector<uint8_t>& rawData) {
    ParsedTexture tex;
    tex.isValid = false;

    if (rawData.size() < sizeof(uint32_t) + sizeof(DDS_HEADER)) {
        Platform::Log("[TEXTURE ERROR]: Размер файла слишком мал для заголовка DDS.");
        return tex;
    }

    uint32_t magic;
    std::memcpy(&magic, rawData.data(), sizeof(uint32_t));
    if (magic != DDS_MAGIC) {
        Platform::Log("[TEXTURE ERROR]: Отсутствует сигнатура 'DDS '.");
        return tex;
    }

    DDS_HEADER header;
    std::memcpy(&header, rawData.data() + sizeof(uint32_t), sizeof(DDS_HEADER));

    tex.width = header.dwWidth;
    tex.height = header.dwHeight;
    tex.mipCount = (header.dwMipMapCount == 0) ? 1 : header.dwMipMapCount;

    // Определение формата компрессии
    uint32_t fourCC = header.ddspf.dwFourCC;
    if (fourCC == 0x31545844) tex.format = TextureFormat::DXT1;       // "DXT1"
    else if (fourCC == 0x33545844) tex.format = TextureFormat::DXT3;  // "DXT3"
    else if (fourCC == 0x35545844) tex.format = TextureFormat::DXT5;  // "DXT5"
    else if (fourCC == 111) tex.format = TextureFormat::BC7;          // "DX10" (упрощенно)
    else tex.format = TextureFormat::RGBA8;

    size_t dataOffset = sizeof(uint32_t) + sizeof(DDS_HEADER);
    
    // Если это DX10 расширенный заголовок
    if (fourCC == 0x30315844) { // "DX10"
        dataOffset += 20; // Пропускаем DDS_HEADER_DXT10
        tex.format = TextureFormat::BC7; // В Centralia большинство DX10 это BC7
    }

    size_t dataSize = rawData.size() - dataOffset;
    tex.pixelData.assign(rawData.begin() + dataOffset, rawData.begin() + dataOffset + dataSize);
    
    tex.isValid = true;
    return tex;
}

// ============================================================================
// SECTION 8: SPECIFIC PARSERS - CUSTOM BINARY MESH FORMAT (.CMESH)
// ============================================================================

#pragma pack(push, 1)
struct CMeshHeader {
    uint32_t magic;         // "CMSH"
    uint32_t version;       // 100
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t boneCount;     // Для Skeletal Animation
    uint32_t submeshCount;
};

struct CMeshVertex {
    float px, py, pz;       // Position
    float nx, ny, nz;       // Normal
    float tx, ty, tz, tw;   // Tangent + Handedness
    float u, v;             // TexCoords
    uint32_t boneIndices;   // 4 байта = 4 кости
    float boneWeights[4];   // Веса костей
};

struct CMeshSubmesh {
    uint32_t indexStart;
    uint32_t indexCount;
    char materialName[64];  // Привязка к материалу
};
#pragma pack(pop)

constexpr uint32_t CMESH_MAGIC = 0x48534D43; // "CMSH"

ParsedModel AssetParser::ParseModel(const std::vector<uint8_t>& rawData) {
    ParsedModel model;
    model.isValid = false;

    if (rawData.size() < sizeof(CMeshHeader)) {
        Platform::Log("[MODEL ERROR]: Размер данных меньше заголовка CMSH.");
        return model;
    }

    const CMeshHeader* header = reinterpret_cast<const CMeshHeader*>(rawData.data());
    
    if (header->magic != CMESH_MAGIC) {
        Platform::Log("[MODEL ERROR]: Неверная сигнатура модели. Ожидалось 'CMSH'.");
        return model;
    }

    size_t offset = sizeof(CMeshHeader);

    // Чтение вершин (Vertices)
    const CMeshVertex* vtxData = reinterpret_cast<const CMeshVertex*>(rawData.data() + offset);
    for (uint32_t i = 0; i < header->vertexCount; ++i) {
        ModelVertex vertex;
        vertex.position = { vtxData[i].px, vtxData[i].py, vtxData[i].pz };
        vertex.normal   = { vtxData[i].nx, vtxData[i].ny, vtxData[i].nz };
        vertex.tangent  = { vtxData[i].tx, vtxData[i].ty, vtxData[i].tz, vtxData[i].tw };
        vertex.texCoords= { vtxData[i].u, vtxData[i].v };
        
        // Извлечение индексов костей
        vertex.boneIDs[0] = (vtxData[i].boneIndices) & 0xFF;
        vertex.boneIDs[1] = (vtxData[i].boneIndices >> 8) & 0xFF;
        vertex.boneIDs[2] = (vtxData[i].boneIndices >> 16) & 0xFF;
        vertex.boneIDs[3] = (vtxData[i].boneIndices >> 24) & 0xFF;

        std::memcpy(vertex.weights, vtxData[i].boneWeights, sizeof(float) * 4);
        
        model.vertices.push_back(vertex);
    }
    offset += header->vertexCount * sizeof(CMeshVertex);

    // Чтение индексов (Indices)
    const uint32_t* idxData = reinterpret_cast<const uint32_t*>(rawData.data() + offset);
    model.indices.assign(idxData, idxData + header->indexCount);
    offset += header->indexCount * sizeof(uint32_t);

    // Чтение Submeshes (Группы материалов)
    const CMeshSubmesh* subData = reinterpret_cast<const CMeshSubmesh*>(rawData.data() + offset);
    for (uint32_t i = 0; i < header->submeshCount; ++i) {
        ModelSubmesh sub;
        sub.indexOffset = subData[i].indexStart;
        sub.indexCount = subData[i].indexCount;
        sub.materialPath = std::string(subData[i].materialName);
        model.submeshes.push_back(sub);
    }

    model.isValid = true;
    return model;
}

} // namespace Centralia