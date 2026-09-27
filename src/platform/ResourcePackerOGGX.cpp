#include "platform/ResourcePackerOGGX.hpp"
#include "platform/Platform.hpp"
#include <fstream>
#include <cstring>
#include <array>

namespace Centralia {

// Предвычисленная на этапе компиляции таблица CRC32 — не тратит такты CPU в игре
constexpr auto GenerateCRC32Table() noexcept {
    std::array<uint32_t, 256> table{};
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
        table[i] = crc;
    }
    return table;
}

uint32_t ResourcePackerOGGX::CalculateCRC32(const std::vector<uint8_t>& data) noexcept {
    static constexpr auto crc32Table = GenerateCRC32Table();
    uint32_t crc = 0xFFFFFFFF;
    for (const uint8_t byte : data) {
        crc = (crc >> 8) ^ crc32Table[(crc ^ byte) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

bool ResourcePackerOGGX::PackResources(const std::vector<std::string>& inputFiles, const std::string& outputPackPath) {
    std::ofstream outArchive(outputPackPath, std::ios::binary);
    if (!outArchive.is_open()) {
        Platform::Log("[OGGX PACKER FATAL]: Ошибка ввода-вывода. Диск заблокирован: " + outputPackPath);
        return false;
    }

    // 1. Формируем заголовок контейнера .oggx
    OGGXHeader header{};
    std::memcpy(header.magic, "OGGX", 4);
    header.version = 1;
    header.fileCount = static_cast<uint32_t>(inputFiles.size());
    header.reservedBuffer = 0;
    outArchive.write(reinterpret_cast<const char*>(&header), sizeof(OGGXHeader));

    // Выделяем пустой блок памяти под будущую таблицу оглавления модов
    const uint64_t tableOffset = outArchive.tellp();
    std::vector<OGGXFileEntry> fileEntries(inputFiles.size());
    outArchive.write(reinterpret_cast<const char*>(fileEntries.data()), fileEntries.size() * sizeof(OGGXFileEntry));

    uint64_t currentPayloadOffset = outArchive.tellp();

    // 2. Побайтовое последовательное сжатие файлов мода в общую кучу
    for (size_t i = 0; i < inputFiles.size(); ++i) {
        std::ifstream fileSource(inputFiles[i], std::ios::binary | std::ios::ate);
        if (!fileSource.is_open()) {
            Platform::Log("[OGGX PACKER ERROR]: Ресурс мода заблокирован или отсутствует: " + inputFiles[i]);
            outArchive.close();
            return false;
        }

        const uint64_t fileSize = fileSource.tellg();
        fileSource.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(fileSize);
        if (fileSize > 0) {
            fileSource.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        }

        // Заполняем метаданные оглавления для быстрого чтения из RAM
        OGGXFileEntry& entry = fileEntries[i];
        std::memset(entry.filePath, 0, sizeof(entry.filePath));
        std::strncpy(entry.filePath, inputFiles[i].c_str(), sizeof(entry.filePath) - 1);
        entry.fileOffset = currentPayloadOffset;
        entry.fileSize = fileSize;
        entry.crc32Checksum = CalculateCRC32(buffer);

        if (fileSize > 0) {
            outArchive.write(reinterpret_cast<const char*>(buffer.data()), fileSize);
        }

        currentPayloadOffset = outArchive.tellp();
        fileSource.close();
    }

    // 3. Возвращаем каретку записи назад и запечатываем готовую таблицу оглавления в архив
    outArchive.seekp(tableOffset);
    outArchive.write(reinterpret_cast<const char*>(fileEntries.data()), fileEntries.size() * sizeof(OGGXFileEntry));
    outArchive.close();

    Platform::Log("[OGGX PACKER]: Контейнер собран номинально. Упаковано файлов ресурсов: " + std::to_string(inputFiles.size()));
    return true;
}

bool ResourcePackerOGGX::ValidatePackIntegrity(const std::string& packPath) {
    std::ifstream inArchive(packPath, std::ios::binary);
    if (!inArchive.is_open()) {
        Platform::Log("[OGGX VALIDATOR ERROR]: Мод-пак поврежден или удален во время игры: " + packPath);
        return false;
    }

    OGGXHeader header;
    inArchive.read(reinterpret_cast<char*>(&header), sizeof(OGGXHeader));

    if (std::memcmp(header.magic, "OGGX", 4) != 0) {
        Platform::Log("[OGGX VALIDATOR FATAL]: Сигнатура файла повреждена. Контейнер заблокирован!");
        inArchive.close();
        return false;
    }

    std::vector<OGGXFileEntry> fileEntries(header.fileCount);
    inArchive.read(reinterpret_cast<char*>(fileEntries.data()), header.fileCount * sizeof(OGGXFileEntry));

    // Мастер-валидатор контрольных сумм CRC32 защищает Ghost-RAM от читеров и битых секторов диска
    for (const auto& entry : fileEntries) {
        inArchive.seekg(entry.fileOffset, std::ios::beg);
        std::vector<uint8_t> buffer(entry.fileSize);
        
        if (entry.fileSize > 0) {
            inArchive.read(reinterpret_cast<char*>(buffer.data()), entry.fileSize);
        }

        if (CalculateCRC32(buffer) != entry.crc32Checksum) {
            Platform::Log("[OGGX VALIDATOR FATAL]: Файл мода '" + std::string(entry.filePath) + "' СКОМПРОМЕТИРОВАН ИЛИ ПОВРЕЖДЕН!");
            inArchive.close();
            return false;
        }
    }

    inArchive.close();
    Platform::Log("[OGGX VALIDATOR]: Проверка контрольных сумм архива '" + packPath + "' выполнена. Ошибок целостности нет.");
    return true;
}

} // namespace Centralia
