#pragma once
#include "core/Math3D.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace Centralia {

// Категории строительства в стиле интерфейса Fallout 76 со скриншота
enum class BuildCategory : uint8_t {
    Doors_Walls,     // Двери и Стены
    Manufacturing,   // Изготовление (Заводы Arknights)
    Turrets_Traps,   // Турели и ловушки
    Defense,         // Оборона
    Generators,      // Генераторы (Энергосеть)
    Power_Connectors,// Разъемы питания
    Lighting         // Освещение (Фонари)
};

struct FactoryStructure {
    uint32_t id;
    std::string name;
    BuildCategory category;
    Vector3D position;
    
    // Энергетический баланс (из механики генераторов на твоем скриншоте)
    int32_t powerProduction = 0; // Сколько производит (например, +3 у Маленького генератора)
    int32_t powerConsumption = 0;// Сколько потребляет заводской цех

    // Механика пассивного крафта ресурсов
    uint32_t outputResourceId = 0;   // Какой ресурс генерирует (Железо, Батареи)
    float productionIntervalSec = 5.0f; // Интервал генерации
    float productionTimer = 0.0f;
    uint32_t amountPerTick = 1;
};

class FactorySystem {
private:
    std::vector<FactoryStructure> m_placedStructures;
    int32_t m_totalPowerGenerated = 0;
    int32_t m_totalPowerConsumed = 0;
    
    // Бюджет строительства (ограничение веса базы на CPU, как шкала "Бюджет" на скриншоте)
    float m_buildBudget = 0.0f;
    const float m_maxBudget = 100.0f;

    void RecalculatePowerGrid();

public:
    FactorySystem() = default;
    ~FactorySystem() = default;

    // Разместить объект на карте в стиле меню Fallout 76
    bool PlaceStructure(uint32_t structureId, const Vector3D& position, class Player& player);

    // Фоновый ежекадровый обсчет работы заводов и энергосети на процессоре (CPU)
    void UpdateFactoriesTick(float deltaTime, class Player& player, const class ClassSystem& classSystem);

    // Геттеры для вывода данных в горизонтальный интерфейс
    int32_t GetCurrentPowerOutput() const { return m_totalPowerGenerated - m_totalPowerConsumed; }
    float GetBudgetPercentage() const { return (m_buildBudget / m_maxBudget) * 100.0f; }
    const std::vector<FactoryStructure>& GetPlacedStructures() const { return m_placedStructures; }
};

} // namespace Centralia
