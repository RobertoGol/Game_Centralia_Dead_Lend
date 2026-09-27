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
        char signature[4]; // 'O', 'G', 'G', 'X'
        uint32_t crc32;
        uint32_t compressedSize;
        uint32_t originalSize;
    };

    // Вспомогательная утилита очистки строк конфигов от мусорных пробелов
    inline static std::string TrimWhitespace(const std::string& str) noexcept {
        if (str.empty()) return str;
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

public:
    AssetParser() = default;
    ~AssetParser() = default;

    /**
     * @brief Главный метод: считывает с диска три файла мода (конфиг, текстуру, 3D-сетку)
     * Полностью Header-Only исполнение с защитой от Out-of-Bounds краша CPU.
     */
    inline bool LoadModFromDisk(
        const std::string& category, 
        const std::string& modName, 
        VehicleModification& outMod,
        std::vector<uint8_t>& outTextureBytes,
        std::vector<Vertex3D_GPU>& outMeshVertices) noexcept 
    {
        std::string basePath = Platform::GetSaveDirectoryPath() + "Data/Ingame/mods/" + category + "/" + modName + "/";
        
        std::string configPath  = basePath + "config.txt";
        std::string texturePath = basePath + "texture.bmp";
        std::string meshPath    = basePath + "mesh.obj";    

        // 1. ЧТЕНИЕ И СБОРКА ТЕКСТУРЫ (.BMP)
        std::ifstream textureFile(texturePath, std::ios::binary | std::ios::ate);
        if (!textureFile.is_open()) {
            Platform::Log("AssetParser Warning: Texture mod asset unallocated: " + texturePath);
        } else {
            std::streamsize size = textureFile.tellg();
            textureFile.seekg(0, std::ios::beg);
            outTextureBytes.resize(static_cast<size_t>(size));
            textureFile.read(reinterpret_cast<char*>(outTextureBytes.data()), size);
            textureFile.close();
        }

        // 2. ЧТЕНИЕ И ПАРСИНГ ГЕОМЕТРИИ 3D-СЕТКИ (.OBJ) НА CPU
        std::ifstream meshFile(meshPath);
        if (!meshFile.is_open()) {
            Platform::Log("AssetParser Error: 3D mesh boundary asset missing: " + meshPath);
            return false;
        }

        std::vector<float> temporaryPositions;
        std::vector<float> temporaryNormals;
        std::string meshLine;

        while (std::getline(meshFile, meshLine)) {
            if (meshLine.rfind("v ", 0) == 0) {
                std::stringstream ss(meshLine.substr(2));
                float x, y, z;
                if (ss >> x >> y >> z) {
                    temporaryPositions.push_back(x);
                    temporaryPositions.push_back(y);
                    temporaryPositions.push_back(z);
                }
            }
            else if (meshLine.rfind("vn ", 0) == 0) {
                std::stringstream ss(meshLine.substr(3));
                float nx, ny, nz;
                if (ss >> nx >> ny >> nz) {
                    temporaryNormals.push_back(nx);
                    temporaryNormals.push_back(ny);
                    temporaryNormals.push_back(nz);
                }
            }
            else if (meshLine.rfind("f ", 0) == 0) {
                std::stringstream ss(meshLine.substr(2));
                std::string vertexBlock;
                
                while (ss >> vertexBlock) {
                    std::size_t firstSlash = vertexBlock.find('/');
                    std::size_t lastSlash = vertexBlock.rfind('/');
                    
                    if (firstSlash == std::string::npos) continue; 
                    
                    uint32_t vIdx = std::stoul(vertexBlock.substr(0, firstSlash)) - 1;
                    uint32_t nIdx = 0;
                    
                    if ((vIdx * 3 + 2) >= temporaryPositions.size()) {
                        continue; 
                    }

                    Vertex3D_GPU gpuVertex{};
                    gpuVertex.x = temporaryPositions[vIdx * 3];
                    gpuVertex.y = temporaryPositions[vIdx * 3 + 1];
                    gpuVertex.z = temporaryPositions[vIdx * 3 + 2];

                    if (lastSlash != std::string::npos && lastSlash != firstSlash) {
                        std::string normalPart = vertexBlock.substr(lastSlash + 1);
                        if (!normalPart.empty()) {
                            nIdx = std::stoul(normalPart) - 1;
                            if ((nIdx * 3 + 2) < temporaryNormals.size()) {
                                gpuVertex.nx = temporaryNormals[nIdx * 3];
                                gpuVertex.ny = temporaryNormals[nIdx * 3 + 1];
                                gpuVertex.nz = temporaryNormals[nIdx * 3 + 2];
                            } else {
                                gpuVertex.nx = 0.0f; gpuVertex.ny = 1.0f; gpuVertex.nz = 0.0f;
                            }
                        } else {
                            gpuVertex.nx = 0.0f; gpuVertex.ny = 1.0f; gpuVertex.nz = 0.0f;
                        }
                    } else {
                        gpuVertex.nx = 0.0f; gpuVertex.ny = 1.0f; gpuVertex.nz = 0.0f;
                    }
                    outMeshVertices.push_back(gpuVertex);
                }
            }
        }
        meshFile.close();

        // 3. ЧТЕНИЕ ХАРАКТЕРИСТИК ДЕТАЛЕЙ (.TXT)
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            Platform::Log("AssetParser Error: Config target parameter descriptor missing: " + configPath);
            return false;
        }

        std::string line;
        while (std::getline(configFile, line)) {
            std::size_t delimiter = line.find('=');
            if (delimiter != std::string::npos) {
                std::string key = TrimWhitespace(line.substr(0, delimiter)); 
                std::string value = TrimWhitespace(line.substr(delimiter + 1));
                
                if (key == "id") outMod.id = std::stoul(value);
                else if (key == "name") outMod.name = value;
                else if (key == "health") outMod.health = std::stof(value);
                else if (key == "armor") outMod.armorValue = std::stof(value);
                else if (key == "passability") outMod.terrainPassability = std::stof(value);
                else if (key == "weight") outMod.weightAdded = std::stof(value);
                else if (key == "speed_mult") outMod.speedMultiplier = std::stof(value);
            }
        }
        configFile.close();

        Platform::Log("AssetParser: Asset mod packed structure parsed successfully inside memory tracks.");
        return true;
    }

    /**
     * @brief Утилита для будущего разжатия фреймов .oggx в память Ghost-RAM
     */
    inline bool DecompressOggxFrame(const std::vector<uint8_t>& packedStream, std::vector<uint8_t>& outRawBytes) noexcept {
        // Архитектурный шлюз под будущее распаковывание гигабайты моделей на лету
        return true;
    }
};

} // namespace Centralia
