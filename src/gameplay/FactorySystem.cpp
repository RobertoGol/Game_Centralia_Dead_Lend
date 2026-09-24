#include "gameplay/FactorySystem.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/ClassSystem.hpp"
#include "platform/Platform.hpp"
#include <algorithm>

namespace Centralia {

void FactorySystem::RecalculatePowerGrid() {
    m_totalPowerGenerated = 0;
    m_totalPowerConsumed = 0;

    for (const auto& structObj : m_placedStructures) {
        m_totalPowerGenerated += structObj.powerProduction;
        m_totalPowerConsumed += structObj.powerConsumption;
    }
}

bool FactorySystem::PlaceStructure(uint32_t structureId, const Vector3D& position) {
    // Проверяем лимит шкалы Бюджета базы
    if (m_buildBudget + 2.5f > m_maxBudget) {
        Platform::Log("[BUILD ERROR]: Превышен лимит бюджета строительства! Процессор заблокировал размещение.");
        return false;
    }

    FactoryStructure newStructure{};
    newStructure.position = position;

    // Настраиваем объекты по твоим чертежам Fallout 76
    if (structureId == 501) { // Маленький генератор
        newStructure.id = 501;
        newStructure.name = "Маленький генератор";
        newStructure.category = BuildCategory::Generators;
        newStructure.powerProduction = 3; // Выдает ровно +3 энергии, как на скриншоте
    } 
    else if (structureId == 502) { // Экстрактор ресурсов (Arknights: Endfield)
        newStructure.id = 502;
        newStructure.name = "Экстрактор железа";
        newStructure.category = BuildCategory::Manufacturing;
        newStructure.powerConsumption = 2; // Требует 2 единицы энергии
        newStructure.outputResourceId = 2002; // ID Концентрата Железа
        newStructure.productionIntervalSec = 4.0f; 
        newStructure.amountPerTick = 1;
    }

    m_placedStructures.push_back(newStructure);
    m_buildBudget += 2.5f; 

    RecalculatePowerGrid();
    Platform::Log("[BUILD]: Размещен объект '" + newStructure.name + "' на сцене базы.");
    return true;
}

void FactorySystem::UpdateFactoriesTick(float deltaTime, Player& player, const ClassSystem& classSystem) {
    bool isPowerGridStarved = (m_totalPowerGenerated < m_totalPowerConsumed);

    // Модификатор скорости крафта от выбранного класса из Log Horizon
    float classBonus = classSystem.GetAttributes().factoryCraftSpeedMultiplier;

    for (auto& structObj : m_placedStructures) {
        if (structObj.category == BuildCategory::Manufacturing && structObj.outputResourceId != 0) {
            
            // Если генераторы не справляются — заводы Arknights останавливаются
            if (isPowerGridStarved) {
                continue; 
            }

            structObj.productionTimer += deltaTime * classBonus;

            if (structObj.productionTimer >= structObj.productionIntervalSec) {
                structObj.productionTimer = 0.0f;

                // Пассивно генерируем ресурсы в инвентарь игрока
                player.AddItem(structObj.outputResourceId, structObj.amountPerTick, 1.0f);
                
                Platform::Log("[FACTORY]: Пассивное производство! Конвейер '" + structObj.name + 
                              "' выдал ресурс ID " + std::to_string(structObj.outputResourceId));
            }
        }
    }
}

} // namespace Centralia
