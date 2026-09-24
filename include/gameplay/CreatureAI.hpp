#pragma once
#include "core/Math3D.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace Centralia {

// Группы видов существ со своими уникальными законами физики урона
enum class CreatureSpecies : uint8_t {
    Humanoid_Organic, // Люди, Эльфы, Дварфы
    Mechanical_Mecha, // Роботы, тяжелые Титаны, Танки
    Mutant_Behemoth,  // Огромные монстры (как Бегемот на гифке)
    Rad_Ghoul         // Радиоактивные гули Пустоши
};

// Зоны поражения / Компоненты
enum class AnatomyPart : uint8_t {
    Head_Sensor,      // Голова человека / Сенсорный блок робота
    Torso_Chassis,    // Корпус / Броня техники
    Left_Leg_Hydraulic,// Левая нога / Привод шасси
    Right_Leg_Hydraulic,// Правая нога / Привод шасси
    Weapon_System_Link // Оружие в руках / Заплечное орудие робота
};

struct AnatomyComponent {
    std::string partName;
    float health = 100.0f;
    float maxHealth = 100.0f;
    bool isSeveredOrDestroyed = false; // Отсечено / полностью уничтожено
};

class Creature {
private:
    uint32_t m_id;
    std::string m_name;
    CreatureSpecies m_species;
    Vector3D m_position;
    
    std::unordered_map<AnatomyPart, AnatomyComponent> m_anatomyProfile;
    float m_totalHealth = 100.0f;
    bool m_isEnraged = false; // Состояние ярости для монстров

public:
    Creature(uint32_t id, const std::string& name, CreatureSpecies species);
    ~Creature() = default;

    // Главный метод распределения урона в зависимости от вида существа
    void ApplyComponentDamage(AnatomyPart part, float rawDamage);

    // Обновление состояния ИИ на CPU (вызывается в Engine::Update)
    void UpdateAiTick(float deltaTime, const Vector3D& playerPos, class MemoryManager& memory);

    // Геттеры
    CreatureSpecies GetSpecies() const { return m_species; }
    const Vector3D& GetPosition() const { return m_position; }
    void SetPosition(const Vector3D& pos) { m_position = pos; }
    float GetTotalHealth() const { return m_totalHealth; }
    bool IsEnraged() const { return m_isEnraged; }
};

} // namespace Centralia
