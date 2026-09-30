#include "AssetParser.hpp"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <zlib.h> // Если используется сжатие блоков BA2

namespace Centralia {

AssetParser::AssetParser() 
    : m_isLoaded(false), m_archiveVersion(0), m_totalFiles(0), m_headerOffset(0) {
}

AssetParser::~AssetParser() {
    CloseArchive();
}

bool AssetParser::OpenArchive(const std::string& filePath) {
    std::lock_guard<std::mutex> lock(m_parserMutex);
    
    CloseArchive();
    m_archivePath = filePath;

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[AssetParser] Error: Failed to open archive file: " << filePath << "\n";
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize < 16) {
        std::cerr << "[AssetParser] Error: File is too small to be a valid BA2 archive: " << filePath << "\n";
        file.close();
        return false;
    }

    // 1. Чтение общего заголовка архива (BTDX формат)
    char magic[4] = {0};
    file.read(magic, 4);
    if (std::string(magic, 4) != "BTDX") {
        std::cerr << "[AssetParser] Error: Invalid archive magic signature (expected 'BTDX').\n";
        file.close();
        return false;
    }

    file.read(reinterpret_cast<char*>(&m_archiveVersion), sizeof(m_archiveVersion));
    
    char archiveType[4] = {0};
    file.read(archiveType, 4); // 'GNRL' (общие файлы) или 'DX10' (текстуры)

    file.read(reinterpret_cast<char*>(&m_totalFiles), sizeof(m_totalFiles));
    file.read(reinterpret_cast<char*>(&m_headerOffset), sizeof(m_headerOffset));

    std::cout << "[AssetParser] Info: Opening archive version " << m_archiveVersion 
              << ", Type: " << std::string(archiveType, 4) 
              << ", Total files: " << m_totalFiles << "\n";

    // 2. Чтение таблицы файлов (File Table Headers)
    file.seekg(static_cast<std::streamoff>(m_headerOffset), std::ios::beg);
    
    m_entries.clear();
    m_entries.reserve(m_totalFiles);

    for (uint32_t i = 0; i < m_totalFiles; ++i) {
        AssetEntry entry;
        
        // Чтение структуры файла в зависимости от типа архива
        if (std::string(archiveType, 4) == "GNRL") {
            file.read(reinterpret_cast<char*>(&entry.fileHash), sizeof(entry.fileHash));
            
            char ext[4] = {0};
            file.read(ext, 4);
            entry.extension = std::string(ext, 4);

            file.read(reinterpret_cast<char*>(&entry.flags), sizeof(entry.flags));
            file.read(reinterpret_cast<char*>(&entry.offset), sizeof(entry.offset));
            file.read(reinterpret_cast<char*>(&entry.packedSize), sizeof(entry.packedSize));
            file.read(reinterpret_cast<char*>(&entry.unpackedSize), sizeof(entry.unpackedSize));
            
            uint16_t dummyFooter = 0;
            file.read(reinterpret_cast<char*>(&dummyFooter), sizeof(dummyFooter));
        } else {
            // Упрощенный парсинг для текстурных архивов DX10
            file.read(reinterpret_cast<char*>(&entry.fileHash), sizeof(entry.fileHash));
            
            char ext[4] = {0};
            file.read(ext, 4);
            entry.extension = std::string(ext, 4);

            file.read(reinterpret_cast<char*>(&entry.flags), sizeof(entry.flags));
            file.read(reinterpret_cast<char*>(&entry.offset), sizeof(entry.offset));
            file.read(reinterpret_cast<char*>(&entry.packedSize), sizeof(entry.packedSize));
            file.read(reinterpret_cast<char*>(&entry.unpackedSize), sizeof(entry.unpackedSize));
            
            // Текстурные архивы имеют дополнительные поля мипмапов
            uint32_t extraTexInfo = 0;
            file.read(reinterpret_cast<char*>(&extraTexInfo), sizeof(extraTexInfo));
        }

        entry.entryIndex = i;
        m_entries.push_back(entry);
    }

    file.close();
    m_isLoaded = true;
    std::cout << "[AssetParser] Success: Successfully parsed " << m_entries.size() << " asset entries.\n";
    return true;
}

bool AssetParser::ExtractAsset(uint32_t fileHash, std::vector<uint8_t>& outData) const {
    std::lock_guard<std::mutex> lock(m_parserMutex);
    
    if (!m_isLoaded) {
        std::cerr << "[AssetParser] Warning: Attempted to extract asset from an unloaded archive.\n";
        return false;
    }

    const AssetEntry* targetEntry = nullptr;
    for (const auto& entry : m_entries) {
        if (entry.fileHash == fileHash) {
            targetEntry = &entry;
            break;
        }
    }

    if (!targetEntry) {
        std::cerr << "[AssetParser] Warning: Asset hash 0x" << std::hex << fileHash << " not found in archive.\n";
        return false;
    }

    std::ifstream file(m_archivePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[AssetParser] Error: Failed to reopen archive for asset extraction: " << m_archivePath << "\n";
        return false;
    }

    file.seekg(static_cast<std::streamoff>(targetEntry->offset), std::ios::beg);

    // Если данные упакованы с использованием чанков или сжатия zlib
    if (targetEntry->packedSize > 0 && targetEntry->packedSize != targetEntry->unpackedSize) {
        std::vector<uint8_t> compressedBuffer(targetEntry->packedSize);
        file.read(reinterpret_cast<char*>(compressedBuffer.data()), targetEntry->packedSize);
        file.close();

        outData.resize(targetEntry->unpackedSize);
        uLongf destLen = targetEntry->unpackedSize;
        
        int res = uncompress(outData.data(), &destLen, compressedBuffer.data(), targetEntry->packedSize);
        if (res != Z_OK) {
            std::cerr << "[AssetParser] Error: Zlib decompression failed with error code: " << res << "\n";
            return false;
        }
    } else {
        // Данные хранятся в распакованном или прямом виде
        outData.resize(targetEntry->unpackedSize > 0 ? targetEntry->unpackedSize : targetEntry->packedSize);
        uint32_t bytesToRead = (targetEntry->unpackedSize > 0) ? targetEntry->unpackedSize : targetEntry->packedSize;
        
        file.read(reinterpret_cast<char*>(outData.data()), bytesToRead);
        file.close();
    }

    return true;
}

bool AssetParser::ExtractAssetByName(const std::string& assetName, std::vector<uint8_t>& outData) const {
    uint32_t computedHash = ComputeBethesdaHash(assetName);
    return ExtractAsset(computedHash, outData);
}

uint32_t AssetParser::ComputeBethesdaHash(const std::string& path) const {
    // Стандартный алгоритм хэширования путей файлов Bethesda (.ba2)
    uint32_t h1 = 0;
    uint32_t h2 = 0;
    uint32_t h3 = 0;

    std::string lowerPath = path;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);
    // Заменяем слеши на обратные для консистентности хэша
    std::replace(lowerPath.begin(), lowerPath.end(), '/', '\\');

    size_t len = lowerPath.length();
    if (len > 0) {
        size_t mid = len / 2;
        for (size_t i = 0; i < mid; ++i) {
            h1 = (h1 * 0x1000193) ^ static_cast<uint8_t>(lowerPath[i]);
        }
        for (size_t i = mid; i < len; ++i) {
            h2 = (h2 * 0x1000193) ^ static_cast<uint8_t>(lowerPath[i]);
        }
    }

    h3 = static_cast<uint32_t>(len);
    return (h1 ^ h3) + h2;
}

bool AssetParser::FileExists(uint32_t fileHash) const {
    std::lock_guard<std::mutex> lock(m_parserMutex);
    for (const auto& entry : m_entries) {
        if (entry.fileHash == fileHash) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> AssetParser::GetLoadedExtensionsList() const {
    std::lock_guard<std::mutex> lock(m_parserMutex);
    std::vector<std::string> extensions;
    for (const auto& entry : m_entries) {
        if (std::find(extensions.begin(), extensions.end(), entry.extension) == extensions.end()) {
            extensions.push_back(entry.extension);
        }
    }
    return extensions;
}

void AssetParser::CloseArchive() noexcept {
    std::lock_guard<std::mutex> lock(m_parserMutex);
    m_entries.clear();
    m_archivePath.clear();
    m_isLoaded = false;
    m_archiveVersion = 0;
    m_totalFiles = 0;
    m_headerOffset = 0;
}

} // namespace Centralia