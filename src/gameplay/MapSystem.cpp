#include "gameplay/MapSystem.hpp"
#include "core/MemoryManager.hpp"
#include "platform/Platform.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/ClassSystem.hpp"
#include <fstream>
#include <cmath>
#include <cstring>

namespace Centralia {

void MapSystem::GenerateDefaultTestMap(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    
    // Инициализируем всю карту (уровень Пустоши Z=51, прочность блоков 100.0, температура 20 градусов)
    MapTile defaultTile{51, SurfaceMaterial::Grass_Dirt, MapObject::None, EnvironmentEffect::None, 100.0f, 20.0f, 0, {0,0,0,0,0,0}};
    m_tileGrid.resize(m_width * m_height, defaultTile);

    // Строим внутренние стены Бункера Обучения (Высота Z=25)
    for (uint32_t x = 10; x < 40; ++x) {
        for (uint32_t z = 10; z < 40; ++z) {
            uint32_t idx = z * m_width + x;
            m_tileGrid[idx].heightLevel = 25; 
            m_tileGrid[idx].material = SurfaceMaterial::Concrete;
            m_tileGrid[idx].tileHealth = 500.0f; // Военный бетон бункера крепче земли
            
            if (x == 10 || x == 39 || z == 10 || z == 39) {
                m_tileGrid[idx].object = MapObject::Bunker_Wall_Block;
            }
        }
    }

    // Привязываем скрытый сюжетный квест "Триггер 13" к ячейке сейфа
    uint32_t safeIdx = 15 * m_width + 16;
    m_tileGrid[safeIdx].object = MapObject::Safe_Old_Photo;
    m_tileGrid[safeIdx].linkedTriggerId = 13; // Вшиваем триггер напрямую в карту!

    // Радиация и ЭМИ-зоны
    m_tileGrid[80 * m_width + 80].effect = EnvironmentEffect::Radiation_Critical;
    m_tileGrid[85 * m_width + 80].effect = EnvironmentEffect::EMP_Annulet;

    Platform::Log("MapSystem: Сгенерирована 22-байтовая тест-карта (Лор-триггеры, Разрушаемость и Тепловой слой активны).");
}

bool MapSystem::LoadMapFromFile(const std::string& filename) {
    std::string fullPath = Platform::GetSaveDirectoryPath() + filename;
    std::ifstream file(fullPath, std::ios::binary);

    if (!file.is_open()) {
        Platform::Log("[MAP]: Файл '" + filename + "' не найден. Создается дефолтная 22-байтовая структура...");
        GenerateDefaultTestMap(256, 256);
        return false;
    }

    file.read(reinterpret_cast<char*>(&m_width), sizeof(m_width));
    file.read(reinterpret_cast<char*>(&m_height), sizeof(m_height));

    m_tileGrid.resize(m_width * m_height);
    
    // Считываем всю 22-байтовую сетку карты test.map одним бинарным блоком
    file.read(reinterpret_cast<char*>(m_tileGrid.data()), m_tileGrid.size() * sizeof(MapTile));
    file.close();

    Platform::Log("[MAP]: Бинарный файл '" + filename + "' успешно прогружен (Размер ячейки: " + 
                  std::to_string(sizeof(MapTile)) + " байт).");
    return true;
}

void MapSystem::ApplyDamageToTile(const Vector3D& position, float damageAmount) {
    int32_t tileX = static_cast<int32_t>(std::floor(position.x + (m_width / 2.0f)));
    int32_t tileZ = static_cast<int32_t>(std::floor(position.z + (m_height / 2.0f)));

    if (tileX < 0 || tileX >= static_cast<int32_t>(m_width) || tileZ < 0 || tileZ >= static_cast<int32_t>(m_height)) return;

    MapTile& tile = m_tileGrid[tileZ * m_width + tileX];
    if (tile.object == MapObject::None) return;

    // Наносим урон прочности блока окружающей среды (Механика разрушаемости)
    tile.tileHealth -= damageAmount;
    
    // Логика нагрева блока при попадании снаряда (Связь с тепловой матрицей)
    tile.thermalLoad += damageAmount * 0.1f; 

    if (tile.tileHealth <= 0.0f) {
        tile.tileHealth = 0.0f;
        tile.object = MapObject::None; // Стена полностью разрушена пушками Титанов!
        tile.material = SurfaceMaterial::Concrete; // На месте стены остается бетонный пол
        Platform::Log("[MAP DESTRUCTION]: Блок в координатах 3D-миры полностью УНИЧТОЖЕН.");
    }
}

bool MapSystem::CheckCollision(const Vector3D& position) const {
    int32_t tileX = static_cast<int32_t>(std::floor(position.x + (m_width / 2.0f)));
    int32_t tileZ = static_cast<int32_t>(std::floor(position.z + (m_height / 2.0f)));

    if (tileX < 0 || tileX >= static_cast<int32_t>(m_width) || tileZ < 0 || tileZ >= static_cast<int32_t>(m_height)) return true;

    const MapTile& tile = m_tileGrid[tileZ * m_width + tileX];
    return (tile.object == MapObject::Bunker_Wall_Block);
}

void MapSystem::UpdateMapEnvironment(float deltaTime, Player& player, ClassSystem& classSystem) {
    Vector3D pos = player.GetPosition();
    int32_t tileX = static_cast<int32_t>(std::floor(pos.x + (m_width / 2.0f)));
    int32_t tileZ = static_cast<int32_t>(std::floor(pos.z + (m_height / 2.0f)));

    if (tileX < 0 || tileX >= static_cast<int32_t>(m_width) || tileZ < 0 || tileZ >= static_cast<int32_t>(m_height)) return;

    const MapTile& tile = m_tileGrid[tileZ * m_width + tileX];
    SurvivalStats& stats = player.GetStats();

    // Проверка вшитого в карту квестового ID ("Триггер 13")
    if (tile.linkedTriggerId == 13 && tile.object == MapObject::Safe_Old_Photo) {
        // Процессор фиксирует, что игрок подошел к квестовой точке
        static bool triggerLogged = false;
        if (!triggerLogged) {
            triggerLogged = true;
            Platform::Log("[STORY TRIGGER]: Обнаружена ячейка карты, привязанная к Триггеру 13!");
        }
    }

    // Дебаффы среды
    if (tile.effect == EnvironmentEffect::Radiation_Critical && m_memoryManager.GetRegistryValue("player_race") != 2) {
        stats.radiation = std::min(100.0f, stats.radiation + (8.0f * deltaTime));
    }
    
    if (tile.effect == EnvironmentEffect::EMP_Annulet && 
        (m_memoryManager.GetRegistryValue("player_race") == 1 || m_memoryManager.GetRegistryValue("player_race") == 2)) 
    {
        stats.thirst = std::max(0.0f, stats.thirst - (10.0f * deltaTime));
    }
}

} // namespace Centralia
