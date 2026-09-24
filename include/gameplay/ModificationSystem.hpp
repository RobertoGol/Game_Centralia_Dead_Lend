#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <algorithm>

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
    std::string loreDescription; // Текстовый лор для погружения в мир игры
    ModSlot slot;
    
    // --- 5 КЛЮЧЕВЫХ МЕХАНИК КОЛЕСА // МОДУЛЯ ---
    float health = 100.0f;          // 1. Текущее здоровье колеса (можно прострелить)
    float armorValue = 5.0f;        // 2. Бронирование колеса (сопротивление урону)
    float terrainPassability = 1.0f;// 3. Проходимость (колеса: грязь -0.2, асфальт +0.3; гусеницы: везде 1.0)
    float weightAdded = 150.0f;     // 4. Вес колеса/детали в кг (влияет на разгон и нагрузку процессора)
    float speedMultiplier = 1.0f;   // 5. Модификатор максимальной скорости техники
};

/ Универсальный шаблон лорного предмета (Фотоаппарат, Снимок из прошлого и т.д.)
struct UniverseItem {
    uint32_t id;
    std::string name;
    std::string loreDescription; // Описание, раскрывающее историю мира
    float weight;
    uint32_t quantity;
    bool isQuestItem;            // Важно для предметов вроде снимка памяти
};

// Интерфейс для всего, что можно модифицировать (Танки, Титаны, Экзоскелеты)
class ModdableEntity {
protected:
    std::vector<Modification> m_installedMods;
    float m_baseSpeed = 5.0f;
    float m_baseArmor = 10.0f;

public:
    virtual ~ModdableEntity() = default;

    bool InstallModification(const Modification& mod) {
        // Проверяем, не занят ли уже этот слот (в простых системах один слот — один мод)
        for (const auto& installed : m_installedMods) {
            if (installed.slot == mod.slot) return false; 
        }
        m_installedMods.push_back(mod);
        return true;
    }

    void RemoveModification(uint32_t modId) {
        m_installedMods.erase(
            std::remove_if(m_installedMods.begin(), m_installedMods.end(),
                [modId](const Modification& m) { return m.id == modId; }),
            m_installedMods.end()
        );
    }

    // Высчитываем итоговую скорость с учетом колес, гусениц или тяжелой брони
    float GetModifiedSpeed() const {
        float speed = m_baseSpeed;
        for (const auto& mod : m_installedMods) {
            speed *= mod.speedMultiplier;
        }
        return speed;
    }
};

} // namespace Centralia
