#pragma once
#include <string>
#include <cstdint>
#include "core/Math3D.hpp"

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

enum class EntityControlMode : uint8_t {
    Pilot_Humanoid,   
    Titan_Vehicle,
    Admin_Observer     
};

struct ClassAttributes {
    float maxHealthModifier = 1.0f;
    float energyRegenRate = 1.0f;
    float factoryCraftSpeedMultiplier = 1.0f;
    
    bool hasGodMode = false;
    bool revealFogOfWar = false;
};

class ClassSystem {
private:
    ActiveControlMode m_sessionControlMode = ActiveControlMode::Standard_Player;
    EngineControlMode m_engineControlMode; 
    EntityControlMode m_currentMode        = EntityControlMode::Pilot_Humanoid;
    HumanClass        m_humanClass         = HumanClass::Guardian_Warrior;
    TitanClass        m_titanClass         = TitanClass::Vanguard_Brawler;
    ClassAttributes   m_currentAttributes;

    uint64_t m_masterAdminHwidHash = 0x8C5A3F11B29DEE77;

public:
    ClassSystem();

    void SetHumanClass(HumanClass hClass);
    void SetTitanClass(TitanClass tClass);

    bool AuthenticateAndActivateAdmin(const std::string& currentDeviceHwid);
    void ToggleControlMode(EntityControlMode mode);

    [[nodiscard]] ActiveControlMode GetControlMode() const noexcept { return m_sessionControlMode; }
    [[nodiscard]] EngineControlMode GetEngineControlMode() const noexcept { return m_engineControlMode; }
    [[nodiscard]] HumanClass GetHumanClass() const noexcept { return m_humanClass; }
    [[nodiscard]] TitanClass GetTitanClass() const noexcept { return m_titanClass; }
    [[nodiscard]] const ClassAttributes& GetAttributes() const noexcept { return m_currentAttributes; }
};

} // namespace Centralia