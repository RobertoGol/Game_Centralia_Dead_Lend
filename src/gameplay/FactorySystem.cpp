#include "gameplay/FactorySystem.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/ClassSystem.hpp"
#include "platform/Platform.hpp"
#include <algorithm>

namespace Centralia {

void FactorySystem::RecalculatePowerGrid() {
    m_totalPowerGenerated = 0;
    m_totalPowerConsumed = 0;

    // Процессор суммирует всю выработку и потребление энергии базы хоста
    for (const auto& structObj : m_placedStructures) {
        m_totalPowerGenerated += structObj.powerProduction;
        m_totalPowerConsumed += structObj.powerConsumption;
    }
}

bool FactorySystem::PlaceStructure(uint32_t structureId, const Vector3D& position, Player& player) {
    // Проверяем лимит шкалы Бюджета базы
    if (m_buildBudget + 2.5f > m_maxBudget) {
        Platform::Log("[BUILD ERROR]: Превышен лимит бюджета строительства! Процессор заблокировал размещение.");
        return false;
    }

    FactoryStructure newStructure{};
    newStructure.position = position;

    // Конфигурируем объекты по чертежам твоего скриншота Fallout 76
    if (structureId == 501) { // Маленький генератор со скриншота
        newStructure.id = 501;
        newStructure.name = "Маленький генератор";
        newStructure.category = BuildCategory::Generators;
        newStructure.powerProduction = 3; // Производит: 3 Энергии (как на скрине)
        
        // Списываем ресурсы из инвентаря игрока: Сталь x4, Медь x2, Шестеренки x2 и т.д.
        // (Логика списания ресурсов хлама привязана к Player::RemoveItem)
    } 
    else if (structureId == 502) { // Автоматический экстрактор железа (Arknights: Endfield)
        newStructure.id = 502;
        newStructure.name = "Экстрактор железа";
        newStructure.category = BuildCategory::Manufacturing;
        newStructure.powerConsumption = 2; // Требует 2 единицы энергии
        newStructure.outputResourceId = 2002; // ID Концентрата Железа из базы лора
        newStructure.productionIntervalSec = 4.0f; // Каждые 4 секунды выдает руду
        newStructure.amountPerTick = 1;
    }

    m_placedStructures.push_back(newStructure);
    m_buildBudget += 2.5f; // Увеличиваем шкалу бюджета

    // Мгновенно пересчитываем энергосеть
    RecalculatePowerGrid();
    Platform::Log("[BUILD]: Размещен объект '" + newStructure.name + "' в координатах 3D сцены.");
    return true;
}

void FactorySystem::UpdateFactoriesTick(float deltaTime, Player& player, const ClassSystem& classSystem) {
    // Проверяем, хватает ли энергии на всю фабрику
    bool isPowerGridStarved = (m_totalPowerGenerated < m_totalPowerConsumed);

    // Модификатор скорости крафта от класса Log Horizon (например, Ассасины собирают быстрее)
    float classBonus = classSystem.GetAttributes().factoryCraftSpeedMultiplier;

    for (auto& structObj : m_placedStructures) {
        // Обсчитываем только производственные цеха категории Изготовление
        if (structObj.category == BuildCategory::Manufacturing && structObj.outputResourceId != 0) {
            
            // Если энергосеть перегружена — заводы Arknights аварийно останавливаются
            if (isPowerGridStarved) {
                continue; 
            }

            // Процессор обновляет таймеры пассивного производства
            structObj.productionTimer += deltaTime * classBonus;

            if (structObj.productionTimer >= structObj.productionIntervalSec) {
                structObj.productionTimer = 0.0f;

                // Пассивно добавляем произведенный ресурс (Железо/Дерево) в инвентарь игрока
                player.AddItem(structObj.outputResourceId, structObj.amountPerTick, 1.0f);
                
                Platform::Log("[FACTORY]: Пассивное производство! Конвейер '" + structObj.name + 
                              "' выдал ресурс ID " + std::to_string(structObj.outputResourceId) + " в инвентарь.");
            }
        }
    }
}

} // namespace Centralia
