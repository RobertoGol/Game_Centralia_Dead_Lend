#pragma once
#include "gameplay/ModificationSystem.hpp" // Предоставляет тип VehicleModification
#include "platform/Platform.hpp"           // Предоставляет лог-систему движка
#include <string>
#include <vector>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace Centralia {

// Структура упакованной 3D-вершины для шейдера отражений (выровнена по ISO C++)
struct Vertex3D_GPU {
    float x, y, z;    // Координаты в пространстве
    float nx, ny, nz; // Нормали для честных динамических отражений "старой школы"
};

class AssetParser {
private:
    // Сигнатура сжатого контейнера (Аналог фреймов OGG для сжатия ресурсов игры)
    struct OggxFrameHeader {
        char signature[4];
        uint32_t crc32;
        uint32_t compressedSize;
        uint32_t originalSize;
    };

    static std::string TrimWhitespace(const std::string& str)  ;

public:
    AssetParser() = default;
    ~AssetParser() = default;

    /**
     * @brief Главный метод: считывает с диска три файла мода (конфиг, текстуру, 3D-сетку)
     * Полностью Header-Only исполнение с защитой от Out-of-Bounds краша CPU.
     */
    bool LoadModFromDisk(
        const std::string& category, 
        const std::string& modName, 
        VehicleModification& outMod,
        std::vector<uint8_t>& outTextureBytes,
        std::vector<Vertex3D_GPU>& outMeshVertices)  ;
    /**
     * @brief Утилита для будущего разжатия фреймов .oggx в память Ghost-RAM
     */
    bool DecompressOggxFrame(const std::vector<uint8_t>& packedStream, std::vector<uint8_t>& outRawBytes);
};

} // namespace Centralia
