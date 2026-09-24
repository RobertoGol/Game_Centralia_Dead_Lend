#pragma once
#include "core/Math3D.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace Centralia {

enum class SurfaceMaterial : uint8_t {
    Grass_Dirt   = 0,
    Concrete     = 1,
    Wood         = 2,
    Rusted_Metal = 3,
    Water_Puddle = 4 
};

enum class MapObject : uint8_t {
    None               = 0,
    Bunker_Wall_Block  = 1,  // Бетонная стена (блокирует WASD)
    Loot_Box_Junk      = 2,  // Ящик с металлоломом
    Terminal_OxN_Init  = 3,  // Стартовый терминал "Ox-n init"
    Titan_Spawn_Pad    = 4,  // Площадка сборки Титанов
    Factory_Conveyor   = 5,  // Конвейерная лента завода Arknights
    Safe_Old_Photo     = 6   // Сейф, где лежит Снимок из прошлого
};

enum class EnvironmentEffect : uint8_t {
    None               = 0,
    Radiation_Low      = 1,  
    Radiation_Critical = 2,  // Смертельная радиация у реактора
    EMP_Annulet        = 3,  // ЭМИ-поле (разряжает Киборгов/Андроидов)
    Acid_Mud           = 4   // Кислотная грязь (разрушает колеса)
};

// Финальная жестко выровненная 22-байтовая структура тайла мира
#pragma pack(push, 1) // Отключаем авто-выравнивание компилятора, фиксируем ровно 22 байта
struct MapTile {
    uint8_t heightLevel;          // 1 байт
    SurfaceMaterial material;     // 1 байт
    MapObject object;             // 1 байт
    EnvironmentEffect effect;     // 1 байт
    float tileHealth;             // 4 байта (разрушаемость окружающей среды)
    float thermalLoad;            // 4 байта (тепловая матрица ячейки)
    uint32_t linkedTriggerId;     // 4 байта (сюжетный "Триггер 13")
    uint8_t reservedBytes[6];     // 6 байт (резервный слой под мехи и танки)
};
#pragma pack(pop)

class MapSystem {
private:
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    std::vector<MapTile> m_tileGrid;

    MapSystem() = default; // Синглтон

public:
    static MapSystem& GetInstance() {
        static MapSystem instance;
        return instance;
    }

    // Инициализация стартовой площадки Бункера Обучения с 22-байтовыми тайлами
    void GenerateDefaultTestMap(uint32_t width = 256, uint32_t height = 256);

    // Высокопроизводительное чтение бинарного файла test.map
    bool LoadMapFromFile(const std::string& filename);

    // Обработка разрушаемости: нанесение урона стене или полу калибром 5.45 или пушкой Титана
    void ApplyDamageToTile(const Vector3D& position, float damageAmount);

    bool CheckCollision(const Vector3D& position) const;

    void UpdateMapEnvironment(float deltaTime, class Player& player, class ClassSystem& classSystem);

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    const std::vector<MapTile>& GetGrid() const { return m_tileGrid; }
};

} // namespace Centralia
