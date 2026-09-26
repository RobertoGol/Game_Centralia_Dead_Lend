#pragma once
#include <string>
#include <cstdint>
#include "core/Engine.hpp" // Добавляем для связки модулей


namespace Centralia {

enum class HumanClass : uint8_t {
    Guardian_Warrior, 
    Assassin_Rogue,   
    Sorcerer_Mage,    
    Cleric_Healer     
};

enum class TitanClass : uint8_t {
    Vanguard_Brawler, 
    Ion_Laser,        
    Scorch_Thermal,   
    Ronin_Stryder     
};

// Расширяем режимы контроля: добавляем скрытый админский хост-класс
// Геймплейный enum пилотирования роботов (переименован для бесконфликтной сборки)
enum class EntityControlMode : uint8_t {

    Pilot_Humanoid,   
    Titan_Vehicle,
    Admin_Observer     // <-- СКРЫТЫЙ КЛАСС: Админ-Модератор (Консольный Фиксатор)

};

struct ClassAttributes {
    float maxHealthModifier = 1.0f;
    float energyRegenRate = 1.0f;
    float factoryCraftSpeedMultiplier = 1.0f;
    // Админские привилегии
    bool hasGodMode = false;
    bool revealFogOfWar = false;
};

class ClassSystem {
private:

    ActiveControlMode m_currentMode = ActiveControlMode::Standard_Player;  // Берем тип ActiveControlMode из Engine.hpp!
    HumanClass m_humanClass = HumanClass::Guardian_Warrior;
    TitanClass m_titanClass = TitanClass::Vanguard_Brawler;
    ClassAttributes m_currentAttributes;    
    uint64_t m_masterAdminHwidHash = 0x8C5A3F11B29DEE77; // Хэш HWID твоего ПК для защиты от взлома

    EngineControlMode m_engineControlMode = EngineControlMode::Standard_Player; // Было m_currentMode     // Синхронизировано с Engine.hpp
    EntityControlMode m_entityControlMode = EntityControlMode::Pilot_Humanoid;  // Было m_currentMode

public:
    ClassSystem() = default;
    void SetHumanClass(HumanClass hClass);
    void SetTitanClass(TitanClass tClass);
    
    // Активация режима Модератора с проверкой безопасности железа
    bool AuthenticateAndActivateAdmin(const std::string& currentDeviceHwid);
    void ToggleControlMode(ActiveControlMode mode);

    ActiveControlMode GetControlMode() const { return m_currentMode; }
    HumanClass GetHumanClass() const { return m_humanClass; }
    TitanClass GetTitanClass() const { return m_titanClass; }
    const ClassAttributes& GetAttributes() const { return m_currentAttributes; }
    
    EntityControlMode GetEntityControlMode() const { return m_entityControlMode; } // Было GetControlMode
    EngineControlMode GetEngineControlMode() const { return m_engineControlMode; } // Было GetControlMode
};

} // namespace Centralia
