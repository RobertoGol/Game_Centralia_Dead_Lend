#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <cstring>
#include <array>
#include "platform/Platform.hpp" // Твоя лог-система

namespace Centralia {

#pragma pack(push, 1)
struct OGGXHeader {
    uint8_t  magic[4];       // "OGGX"
    uint32_t version;        
    uint32_t fileCount;      
    uint32_t reservedBuffer; 
};

struct OGGXFileEntry {
    char     filePath[128];  
    uint64_t fileOffset;     
    uint64_t fileSize;       
    uint32_t crc32Checksum;  
};
#pragma pack(pop)

class ResourcePackerOGGX {
private:
    // Генерация предвычисленной таблицы CRC32 на этапе компиляции
    constexpr static auto GenerateCRC32Table() noexcept {
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

    inline static uint32_t CalculateCRC32(const std::vector<uint8_t>& data) noexcept {
        static constexpr auto crc32Table = GenerateCRC32Table();
        uint32_t crc = 0xFFFFFFFF;
        for (const uint8_t byte : data) {
            crc = (crc >> 8) ^ crc32Table[(crc ^ byte) & 0xFF];
        }
        return crc ^ 0xFFFFFFFF;
    }

public:
    ResourcePackerOGGX() = default;
    ~ResourcePackerOGGX() = default;

    inline static bool PackResources(const std::vector<std::string>& inputFiles, const std::string& outputPackPath) {
        std::ofstream outArchive(outputPackPath, std::ios::binary);
        if (!outArchive.is_open()) {
            Platform::Log("[OGGX PACKER ERROR]: Не удалось создать архив: " + outputPackPath);
            return false;
        }

        OGGXHeader header{};
        std::memcpy(header.magic, "OGGX", 4);
        header.version = 1;
        header.fileCount = static_cast<uint32_t>(inputFiles.size());
        header.reservedBuffer = 0;
        outArchive.write(reinterpret_cast<const char*>(&header), sizeof(OGGXHeader));

        const uint64_t tableOffset = outArchive.tellp();
        std::vector<OGGXFileEntry> fileEntries(inputFiles.size());
        outArchive.write(reinterpret_cast<const char*>(fileEntries.data()), fileEntries.size() * sizeof(OGGXFileEntry));

        uint64_t currentPayloadOffset = outArchive.tellp();

        for (size_t i = 0; i < inputFiles.size(); ++i) {
            std::ifstream fileSource(inputFiles[i], std::ios::binary | std::ios::ate);
            if (!fileSource.is_open()) {
                Platform::Log("[OGGX PACKER ERROR]: Нет файла ресурса: " + inputFiles[i]);
                outArchive.close();
                return false;
            }

            const uint64_t fileSize = fileSource.tellg();
            fileSource.seekg(0, std::ios::beg);

            std::vector<uint8_t> buffer(fileSize);
            if (fileSize > 0) {
                fileSource.read(reinterpret_cast<char*>(buffer.data()), fileSize);
            }

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

        outArchive.seekp(tableOffset);
        outArchive.write(reinterpret_cast<const char*>(fileEntries.data()), fileEntries.size() * sizeof(OGGXFileEntry));
        outArchive.close();

        Platform::Log("[OGGX PACKER]: Сжато ресурсов: " + std::to_string(inputFiles.size()) + " в контейнер " + outputPackPath);
        return true;
    }

    inline static bool ValidatePackIntegrity(const std::string& packPath) {
        std::ifstream inArchive(packPath, std::ios::binary);
        if (!inArchive.is_open()) {
            Platform::Log("[OGGX VALIDATOR ERROR]: Архив не найден: " + packPath);
            return false;
        }

        OGGXHeader header;
        inArchive.read(reinterpret_cast<char*>(&header), sizeof(OGGXHeader));

        if (std::memcmp(header.magic, "OGGX", 4) != 0) {
            Platform::Log("[OGGX VALIDATOR FATAL]: Битый заголовок OGGX архива!");
            inArchive.close();
            return false;
        }

        std::vector<OGGXFileEntry> fileEntries(header.fileCount);
        inArchive.read(reinterpret_cast<char*>(fileEntries.data()), header.fileCount * sizeof(OGGXFileEntry));

        for (const auto& entry : fileEntries) {
            inArchive.seekg(entry.fileOffset, std::ios::beg);
            std::vector<uint8_t> buffer(entry.fileSize);
            
            if (entry.fileSize > 0) {
                inArchive.read(reinterpret_cast<char*>(buffer.data()), entry.fileSize);
            }

            if (CalculateCRC32(buffer) != entry.crc32Checksum) {
                Platform::Log("[OGGX VALIDATOR CORRUPTION]: Файл '" + std::string(entry.filePath) + "' ПОВРЕЖДЕН!");
                inArchive.close();
                return false;
            }
        }

        inArchive.close();
        Platform::Log("[OGGX VALIDATOR NOMINAL]: Пакет " + packPath + " успешно прошел проверку CRC32.");
        return true;
    }
};

} // namespace Centralia
