#pragma once
#include "gameplay/ModificationSystem.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace Centralia {

// Структура упакованной 3D-вершины для шейдера отражений
struct Vertex3D_GPU {
    float x, y, z;    // Координаты в пространстве
    float nx, ny, nz; // Нормали для честных динамических отражений "старой школы"
};

class AssetParser {
private:
    // Сигнатура сжатого контейнера (Аналог фреймов OGG для сжатия ресурсов игры)
    struct OggxFrameHeader {
        char signature[4]; // 'O', 'G', 'G', 'X'
        uint32_t crc32;
        uint32_t compressedSize;
        uint32_t originalSize;
    };

public:
    AssetParser() = default;
    ~AssetParser() = default;

    // Главный метод: считывает с диска Windows 10 три файла мода (конфиг, текстуру, 3D-сетку)
    bool LoadModFromDisk(
        const std::string& category, 
        const std::string& modName, 
        VehicleModification& outMod,
        std::vector<uint8_t>& outTextureBytes,
        std::vector<Vertex3D_GPU>& outMeshVertices
    );

    // Утилита для будущего разжатия фреймов .oggx в память Ghost-RAM
    bool DecompressOggxFrame(const std::vector<uint8_t>& packedStream, std::vector<uint8_t>& outRawBytes);
};

} // namespace Centralia
