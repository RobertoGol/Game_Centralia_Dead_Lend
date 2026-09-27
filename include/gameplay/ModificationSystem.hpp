#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace Centralia {

// Типы слотов для модификаций (куда можно установить деталь)
enum class ModSlot : uint8_t {
    Chassis_Wheel_Track,  // Слот ходовой: колеса, гусеницы, лапы Титанов
    Armor_Plating,        // Слот защиты: навесная броня, экраны
    Weapon_Barrel_Mag,    // Слот оружия: стволы, магазины, ускорители
    Internal_Engine_Core  // Слот ядра: движки, реакторы, батареи Пип-боя
};

// Структура конкретного модуля модификации
struct Modification {
    uint32_t id;
    std::string name;
    ModSlot slot;
    
    // Влияние мода на физику и характеристики (могут быть отрицательными для баланса)
    float speedMultiplier = 1.0f;       // Например, колеса дают 1.25х к скорости, но снижают проходимость
    float weightAdded = 0.0f;           // Добавочный вес детали
    float damageResistanceMod = 0.0f;   // Прибавка к защите
    float energyConsumptionMod = 0.0f;  // Нагрузка на генератор/батарею
};

// Расширенная структура модификации ходовой (Колеса / Гусеницы) и лора
struct VehicleModification {
    uint32_t id;
    std::string name;
    std::string loreDescription;    // Текстовый лор для погружения в мир игры
    ModSlot slot;
    
    // --- 5 КЛЮЧЕВЫХ МЕХАНИК КОЛЕСА / МОДУЛЯ ---
    float health = 100.0f;          // 1. Текущее здоровье колеса (можно прострелить)
    float armorValue = 5.0f;        // 2. Бронирование колеса (сопротивление урону)
    float terrainPassability = 1.0f;// 3. Проходимость (колеса: грязь -0.2, асфальт +0.3; гусеницы: везде 1.0)
    float weightAdded = 150.0f;     // 4. Вес колеса/детали в кг (влияет на разгон)
    float speedMultiplier = 1.0f;   // 5. Модификатор максимальной скорости техники
    float tirePressurePsi = 32.0f;  // 6. Текущее давление в шинах (min 0.0f max 32.0f)
    bool isDetached = false;        // 7. Текущее состояние колеса (полного отрыва/разрушения)
};

// Универсальный шаблон лорного предмета (Фотоаппарат, Снимок из прошлого)
struct UniverseItem {
    uint32_t t_id;
    std::string name;
    std::string loreDescription;
    float weight;
    uint32_t quantity;
    bool isQuestItem;
};

// Интерфейс для всего, что можно модифицировать (Танки, Титаны, Экзоскелеты)
class ModdableEntity {
protected:
    std::vector<Modification> m_installedMods;
    float m_baseSpeed = 5.0f;
    float m_baseArmor = 10.0f;

public:
    virtual ~ModdableEntity() = default;

    // Установка детали в слот хоста базы
    bool InstallModification(const Modification& mod);

    // Снятие детали
    void RemoveModification(uint32_t modId); 

    // Высчитываем итоговую скорость с учетом колес, гусениц или тяжелой брони
    [[nodiscard]] float GetModifiedSpeed() const noexcept;
};

} // namespace Centralia
