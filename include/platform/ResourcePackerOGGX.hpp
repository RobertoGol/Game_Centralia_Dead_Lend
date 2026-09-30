#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <cstring>
#include <array>
#include "platform/Platform.hpp"

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

    static uint32_t CalculateCRC32(const std::vector<uint8_t>& data) noexcept;

public:
    ResourcePackerOGGX() = default;
    ~ResourcePackerOGGX() = default;

    static bool PackResources(const std::vector<std::string>& inputFiles, const std::string& outputPackPath);
    static bool ValidatePackIntegrity(const std::string& packPath);
};

} // namespace Centralia