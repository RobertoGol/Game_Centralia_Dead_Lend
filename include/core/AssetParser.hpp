#pragma once
#include "gameplay/ModificationSystem.hpp"
#include "platform/Platform.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace Centralia {

struct Vertex3D_GPU {
    float x, y, z;
    float nx, ny, nz;
};

class AssetParser {
private:
    struct OggxFrameHeader {
        char signature[4];
        uint32_t crc32;
        uint32_t compressedSize;
        uint32_t originalSize;
    };

    static std::string TrimWhitespace(const std::string& str);

public:
    AssetParser() = default;
    ~AssetParser() = default;

    bool LoadModFromDisk(
        const std::string& category, 
        const std::string& modName, 
        VehicleModification& outMod,
        std::vector<uint8_t>& outTextureBytes,
        std::vector<Vertex3D_GPU>& outMeshVertices);

    bool DecompressOggxFrame(const std::vector<uint8_t>& packedStream, std::vector<uint8_t>& outRawBytes);
};

} // namespace Centralia