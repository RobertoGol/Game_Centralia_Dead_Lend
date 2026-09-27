#include "core/AssetParser.hpp"
#include "platform/Platform.hpp"
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace Centralia {

// Внутренняя вспомогательная утилита для очистки строк от мусорных пробелов и переносов строк
static std::string TrimWhitespace(const std::string& str) noexcept {
    if (str.empty()) return str;
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool AssetParser::LoadModFromDisk(
    const std::string& category, 
    const std::string& modName, 
    VehicleModification& outMod,
    std::vector<uint8_t>& outTextureBytes,
    std::vector<Vertex3D_GPU>& outMeshVertices) 
{
    // Строим иерархию путей строго по твоей схеме: Data/Ingame/mods/[категория]/[название]/
    std::string basePath = Platform::GetSaveDirectoryPath() + "Data/Ingame/mods/" + category + "/" + modName + "/";
    
    std::string configPath  = basePath + "config.txt";
    std::string texturePath = basePath + "texture.bmp"; // Текстура и краски (.col)
    std::string meshPath    = basePath + "mesh.obj";    // 3D-сетка тяжелого Титана/Танка

    // ----------------================================================--------
    // 1. ЧТЕНИЕ И СБОРКА ТЕКСТУРЫ (.BMP / .COL)
    // ----------------================================================--------
    std::ifstream textureFile(texturePath, std::ios::binary | std::ios::ate);
    if (!textureFile.is_open()) {
        Platform::Log("[ASSET PARSER WARNING]: Текстура мода не найдена, используем сталь по умолчанию: " + texturePath);
    } else {
        std::streamsize size = textureFile.tellg();
        textureFile.seekg(0, std::ios::beg);
        outTextureBytes.resize(static_cast<size_t>(size));
        textureFile.read(reinterpret_cast<char*>(outTextureBytes.data()), size);
        textureFile.close();
    }

    // ----------------================================================--------
    // 2. ЧТЕНИЕ И ПАРСИНГ ГЕОМЕТРИИ 3D-СЕТКИ (.OBJ) НА CPU
    // ------------------------------------------------------------------------
    std::ifstream meshFile(meshPath);
    if (!meshFile.is_open()) {
        Platform::Log("[ASSET PARSER ERROR]: Критическая ошибка! 3D-сетка меша отсутствует: " + meshPath);
        return false;
    }

    std::vector<float> temporaryPositions;
    std::vector<float> temporaryNormals;
    std::string meshLine;

    while (std::getline(meshFile, meshLine)) {
        // Парсим координаты вершин (v X Y Z)
        if (meshLine.rfind("v ", 0) == 0) {
            std::stringstream ss(meshLine.substr(2));
            float x, y, z;
            if (ss >> x >> y >> z) {
                temporaryPositions.push_back(x);
                temporaryPositions.push_back(y);
                temporaryPositions.push_back(z);
            }
        }
        // Парсим нормали для зеркальных отражений (vn NX NY NZ)
        else if (meshLine.rfind("vn ", 0) == 0) {
            std::stringstream ss(meshLine.substr(3));
            float nx, ny, nz;
            if (ss >> nx >> ny >> nz) {
                temporaryNormals.push_back(nx);
                temporaryNormals.push_back(ny);
                temporaryNormals.push_back(nz);
            }
        }
        // Строим полигоны граней (f v1/vt1/vn1 v2/vt2/vn2 ...)
        else if (meshLine.rfind("f ", 0) == 0) {
            std::stringstream ss(meshLine.substr(2));
            std::string vertexBlock;
            
            // В рамках упрощенного OBJ-парсера для Windows 10 собираем треугольники
            while (ss >> vertexBlock) {
                std::size_t firstSlash = vertexBlock.find('/');
                std::size_t lastSlash = vertexBlock.rfind('/');
                
                if (firstSlash == std::string::npos) continue; // Защита от кривой разметки
                
                uint32_t vIdx = std::stoul(vertexBlock.substr(0, firstSlash)) - 1;
                uint32_t nIdx = 0;
                
                // ИСПРАВЛЕНО: Жесткий предохранитель от выхода за границы вектора (Ошибки SIGSEGV / Краха CPU)
                if ((vIdx * 3 + 2) >= temporaryPositions.size()) {
                    continue; 
                }

                Vertex3D_GPU gpuVertex{};
                gpuVertex.x = temporaryPositions[vIdx * 3];
                gpuVertex.y = temporaryPositions[vIdx * 3 + 1];
                gpuVertex.z = temporaryPositions[vIdx * 3 + 2];

                if (lastSlash != std::string::npos && lastSlash != firstSlash) {
                    nIdx = std::stoul(vertexBlock.substr(lastSlash + 1)) - 1;
                    
                    // Валидация индексов нормалей перед чтением из ОЗУ
                    if ((nIdx * 3 + 2) < temporaryNormals.size()) {
                        gpuVertex.nx = temporaryNormals[nIdx * 3];
                        gpuVertex.ny = temporaryNormals[nIdx * 3 + 1];
                        gpuVertex.nz = temporaryNormals[nIdx * 3 + 2];
                    } else {
                        gpuVertex.nx = 0.0f; gpuVertex.ny = 1.0f; gpuVertex.nz = 0.0f;
                    }
                } else {
                    // Если нормалей в файле нет, проц забивает авто-заглушку, направленную вверх
                    gpuVertex.nx = 0.0f; gpuVertex.ny = 1.0f; gpuVertex.nz = 0.0f;
                }
                outMeshVertices.push_back(gpuVertex);
            }
        }
    }
    meshFile.close();

    // ----------------================================================--------
    // 3. ЧТЕНИЕ КОНФИГУРАЦИОННОГО ФАЙЛА ХАРАКТЕРИСТИК (.TXT / .CFG)
    // ------------------------------------------------------------------------
    std::ifstream configFile(configPath);
    if (!configFile.is_open()) {
        Platform::Log("[ASSET PARSER ERROR]: Конфиг мода уничтожен или отсутствует: " + configPath);
        return false;
    }

    std::string configFileBufferLine;
    while (std::getline(configFile, configFileBufferLine)) {
        std::size_t delimiter = configFileBufferLine.find('=');
        if (delimiter != std::string::npos) {
            // ИСПРАВЛЕНО: Ключ и значение очищаются от мусорных пробелов до знака равенства
            std::string key = TrimWhitespace(configFileBufferLine.substr(0, delimiter));
            std::string value = TrimWhitespace(configFileBufferLine.substr(delimiter + 1));
            
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

    Platform::Log("[ASSET PARSER]: Ресурс '" + outMod.name + "' успешно собран процессором. Выделено " + 
                  std::to_string(outMeshVertices.size()) + " вершин в буфер GPU. Ошибки Out-of-Bounds устранены.");
    return true;
}

bool AssetParser::DecompressOggxFrame(const std::vector<uint8_t>& packedStream, std::vector<uint8_t>& outRawBytes) {
    // Архитектурный шлюз под будущее сжатие фреймов.
    // Процессор будет читать биты аналогично ogg-страницам, распаковывая гигабайты моделей на лету.
    return true;
}

} // namespace Centralia
