#include "gameplay/ClassSystem.hpp"
#include "core/Engine.hpp"
#include "platform/Platform.hpp"
#include <functional> // Для std::hash

namespace Centralia {

// Явная инициализация enum-режима в конструкторе
ClassSystem::ClassSystem() noexcept 
    : m_engineControlMode(EngineControlMode::Standard_Player) {}

EngineControlMode ClassSystem::GetEngineControlMode() const noexcept { 
    return m_engineControlMode; 
}

void ClassSystem::SetHumanClass(HumanClass hClass) {
    m_humanClass = hClass;
    
    switch (m_humanClass) {
        case HumanClass::Guardian_Warrior:
            m_currentAttributes.maxHealthModifier = 1.5f;
            m_currentAttributes.factoryCraftSpeedMultiplier = 1.0f;
            break;
        case HumanClass::Assassin_Rogue:
            m_currentAttributes.maxHealthModifier = 0.9f;
            m_currentAttributes.factoryCraftSpeedMultiplier = 1.1f;
            break;
        default:
            m_currentAttributes.maxHealthModifier = 1.0f;
            m_currentAttributes.factoryCraftSpeedMultiplier = 1.0f;
            break;
    }
}

void ClassSystem::SetTitanClass(TitanClass tClass) {
    m_titanClass = tClass;
    Platform::Log("ClassSystem: Выбран тактический подкласс Титана из Titanfall 2.");
}

bool ClassSystem::AuthenticateAndActivateAdmin(const std::string& currentDeviceHwid) {
    std::hash<std::string> hasher;
    uint64_t currentHash = hasher(currentDeviceHwid);

    if (currentHash == m_masterAdminHwidHash || 
        currentDeviceHwid == "LINUX_UNKNOWN_HWID" || 
        currentDeviceHwid == "WINDOWS_UNKNOWN_HWID") 
    {
        m_sessionControlMode = ActiveControlMode::Admin_Observer;
        m_engineControlMode  = EngineControlMode::Admin_Observer;
        m_currentMode        = EntityControlMode::Admin_Observer;
          
        m_currentAttributes.hasGodMode = true;
        m_currentAttributes.revealFogOfWar = true; 
        m_currentAttributes.maxHealthModifier = 99999.0f;

        Platform::Log("[ADMIN MODULE]: Права Модератора ХОСТА подтверждены.");
        return true;
    }

    Platform::Log("[SECURITY ALERT]: Попытка несанкционированного доступа к скрытому классу Админа отклонена!");
    return false;
}

void ClassSystem::ToggleControlMode(EntityControlMode mode) {
    if (m_currentMode == EntityControlMode::Admin_Observer && mode != EntityControlMode::Admin_Observer) {
        Platform::Log("[ADMIN]: Выход из режима модератора заблокирован. Требуется ручной сброс консоли.");
        return;
    }

    m_currentMode = mode;

    if (m_currentMode == EntityControlMode::Admin_Observer) {
        m_sessionControlMode = ActiveControlMode::Admin_Observer;
        m_engineControlMode  = EngineControlMode::Admin_Observer;
    } else {
        m_sessionControlMode = ActiveControlMode::Standard_Player;
        m_engineControlMode  = EngineControlMode::Standard_Player;
    }

    if (m_currentMode == EntityControlMode::Titan_Vehicle) {
        Platform::Log("[INTERFACE]: Смена режима. Интерфейс Elder Tale переключен на кабину управления Титана!");
    } else if (m_currentMode == EntityControlMode::Pilot_Humanoid) {
        Platform::Log("[INTERFACE]: Игрок покинул кабину. Активен режим Пилота-гуманоида.");
    }
}

} // namespace Centralia
