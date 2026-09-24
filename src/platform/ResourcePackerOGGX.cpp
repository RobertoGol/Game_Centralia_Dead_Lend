#include "platform/ResourcePackerOGGX.hpp"
#include "platform/Platform.hpp"
#include <fstream>
#include <cstring>
#include <array>

namespace Centralia {

// Генерация предвычисленной таблицы CRC32 на этапе компиляции для максимального FPS
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
        Platform::Log("[OGGX PACKER ERROR]: Не удалось создать архивный файл: " + outputPackPath);
        return false;
    }

    // 1. Формируем и пишем базовый заголовок контейнера
    OGGXHeader header{};
    std::memcpy(header.magic, "OGGX", 4);
    header.version = 1;
    header.fileCount = static_cast<uint32_t>(inputFiles.size());
    header.reservedBuffer = 0;
    outArchive.write(reinterpret_cast<const char*>(&header), sizeof(OGGXHeader));

    // Выделяем место под оглавление архива (File TableEntries), запишем его позже, когда узнаем смещения
    const uint64_t tableOffset = outArchive.tellp();
    std::vector<OGGXFileEntry> fileEntries(inputFiles.size());
    outArchive.write(reinterpret_cast<const char*>(fileEntries.data()), fileEntries.size() * sizeof(OGGXFileEntry));

    // 2. Побайтово упаковываем файлы и считаем CRC32
    uint64_t currentPayloadOffset = outArchive.tellp();

    for (size_t i = 0; i < inputFiles.size(); ++i) {
        std::ifstream fileSource(inputFiles[i], std::ios::binary | std::ios::ate);
        if (!fileSource.is_open()) {
            Platform::Log("[OGGX PACKER ERROR]: Не удалось открыть исходный файл ресурса: " + inputFiles[i]);
            return false;
        }

        const uint64_t fileSize = fileSource.tellg();
        fileSource.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(fileSize);
        if (fileSize > 0) {
            fileSource.read(reinterpret_cast<char*>(buffer.data()), fileSize);
        }

        // Заполняем метаданные для таблицы оглавления
        OGGXFileEntry& entry = fileEntries[i];
        std::memset(entry.filePath, 0, sizeof(entry.filePath));
        std::strncpy(entry.filePath, inputFiles[i].c_str(), sizeof(entry.filePath) - 1);
        entry.fileOffset = currentPayloadOffset;
        entry.fileSize = fileSize;
        entry.crc32Checksum = CalculateCRC32(buffer);

        // Пишем сырое тело файла в общую кучу
        if (fileSize > 0) {
            outArchive.write(reinterpret_cast<const char*>(buffer.data()), fileSize);
        }

        currentPayloadOffset = outArchive.tellp();
        fileSource.close();
    }

    // 3. Возвращаемся в начало и перезаписываем корректную заполненную таблицу оглавления
    outArchive.seekp(tableOffset);
    outArchive.write(reinterpret_cast<const char*>(fileEntries.data()), fileEntries.size() * sizeof(OGGXFileEntry));
    outArchive.close();

    Platform::Log("[OGGX PACKER]: Успешно сжато ресурсов: " + std::to_string(inputFiles.size()) + " в контейнер " + outputPackPath);
    return true;
}

bool ResourcePackerOGGX::ValidatePackIntegrity(const std::string& packPath) {
    std::ifstream inArchive(packPath, std::ios::binary);
    if (!inArchive.is_open()) {
        Platform::Log("[OGGX VALIDATOR ERROR]: Не удалось открыть архив для проверки: " + packPath);
        return false;
    }

    OGGXHeader header;
    inArchive.read(reinterpret_cast<char*>(&header), sizeof(OGGXHeader));

    if (std::memcmp(header.magic, "OGGX", 4) != 0) {
        Platform::Log("[OGGX VALIDATOR FATAL]: Неверная сигнатура файла архива!");
        return false;
    }

    std::vector<OGGXFileEntry> fileEntries(header.fileCount);
    inArchive.read(reinterpret_cast<char*>(fileEntries.data()), header.fileCount * sizeof(OGGXFileEntry));

    // Прогоняем каждый файл внутри кучи через CRC32 валидатор
    for (const auto& entry : fileEntries) {
        inArchive.seekg(entry.fileOffset, std::ios::beg);
        std::vector<uint8_t> buffer(entry.fileSize);
        
        if (entry.fileSize > 0) {
            inArchive.read(reinterpret_cast<char*>(buffer.data()), entry.fileSize);
        }

        uint32_t currentCrc = CalculateCRC32(buffer);
        if (currentCrc != entry.crc32Checksum) {
            Platform::Log("[OGGX VALIDATOR CORRUPTION]: Файл '" + std::string(entry.filePath) + "' ПОВРЕЖДЕН! Контрольные суммы не совпали.");
            return false;
        }
    }

    inArchive.close();
    Platform::Log("[OGGX VALIDATOR NOMINAL]: Пакет ресурсов " + packPath + " успешно прошел CRC32 валидацию. Ошибок нет.");
    return true;
}

} // namespace Centralia
