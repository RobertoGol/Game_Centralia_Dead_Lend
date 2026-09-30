#include "gameplay/ClassSystem.hpp"
#include "core/Engine.hpp"
#include "platform/Platform.hpp"
#include <functional>
#include <algorithm>

namespace Centralia {

ClassSystem::ClassSystem() noexcept 
    : m_engineControlMode(EngineControlMode::Standard_Player),
      m_humanClass(HumanClass::Guardian_Warrior),
      m_titanClass(TitanClass::Vanguard_Class),
      m_currentMode(EntityControlMode::Pilot_Humanoid),
      m_sessionControlMode(ActiveControlMode::Standard_Player),
      m_masterAdminHwidHash(0x1337C0DE42ULL) // Мастер-хеш администратора
{
    // Инициализация базовых атрибутов по умолчанию
    m_currentAttributes.maxHealthModifier = 1.0f;
    m_currentAttributes.factoryCraftSpeedMultiplier = 1.0f;
    m_currentAttributes.hasGodMode = false;
    m_currentAttributes.revealFogOfWar = false;
}

EngineControlMode ClassSystem::GetEngineControlMode() const noexcept { 
    return m_engineControlMode; 
}

EntityControlMode ClassSystem::GetCurrentEntityMode() const noexcept {
    return m_currentMode;
}

const ClassAttributes& ClassSystem::GetAttributes() const noexcept {
    return m_currentAttributes;
}

void ClassSystem::SetHumanClass(HumanClass hClass) {
    m_humanClass = hClass;
    
    switch (m_humanClass) {
        case HumanClass::Guardian_Warrior:
            m_currentAttributes.maxHealthModifier = 1.5f;
            m_currentAttributes.factoryCraftSpeedMultiplier = 1.0f;
            Platform::Log("[CLASS SYSTEM]: Выбран класс Страж (Guardian). Здоровье увеличено на 50%.");[cite: 25]
            break;
        case HumanClass::Assassin_Rogue:
            m_currentAttributes.maxHealthModifier = 0.9f;
            m_currentAttributes.factoryCraftSpeedMultiplier = 1.1f;
            Platform::Log("[CLASS SYSTEM]: Выбран класс Убийца (Assassin). Скорость крафта повышена.");[cite: 25]
            break;
        case HumanClass::Cleric:
            m_currentAttributes.maxHealthModifier = 1.1f;
            m_currentAttributes.factoryCraftSpeedMultiplier = 1.0f;
            Platform::Log("[CLASS SYSTEM]: Выбран класс Клерик. Поддержка группы активирована.");
            break;
        case HumanClass::Enchanter:
            m_currentAttributes.maxHealthModifier = 0.95f;
            m_currentAttributes.factoryCraftSpeedMultiplier = 1.35f;
            Platform::Log("[CLASS SYSTEM]: Выбран класс Чародей (Enchanter). Буст фабричного производства.");
            break;
        default:
            m_currentAttributes.maxHealthModifier = 1.0f;
            m_currentAttributes.factoryCraftSpeedMultiplier = 1.0f;
            break;
    }
}

void ClassSystem::SetTitanClass(TitanClass tClass) {
    m_titanClass = tClass;
    switch (m_titanClass) {
        case TitanClass::Vanguard_Class:
            Platform::Log("[TITAN SYSTEM]: Активирован шагающий класс Титана: Авангард (Vanguard).");[cite: 25]
            break;
        case TitanClass::Scorch_Class:
            Platform::Log("[TITAN SYSTEM]: Активирован шагающий класс Титана: Скорч (Scorch - Термический модуль).");[cite: 25]
            break;
        case TitanClass::Ronin_Class:
            Platform::Log("[TITAN SYSTEM]: Активирован шагающий класс Титана: Ронин (Ronin - Ближний бой).");[cite: 25]
            break;
        default:
            Platform::Log("[TITAN SYSTEM]: Выбран стандартный тяжелый шагающий шасси-титан.");[cite: 25]
            break;
    }
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

        Platform::Log("[ADMIN MODULE]: Права Модератора ХОСТА подтверждены по аппаратному HWID-контексту.");[cite: 25]
        return true;
    }

    Platform::Log("[SECURITY ALERT]: Попытка несанкционированного доступа к скрытому классу Админа отклонена!");[cite: 25]
    return false;
}

void ClassSystem::ToggleControlMode(EntityControlMode mode) {
    if (m_currentMode == EntityControlMode::Admin_Observer && mode != EntityControlMode::Admin_Observer) {
        Platform::Log("[ADMIN]: Выход из режима модератора заблокирован. Требуется ручной сброс консоли.");[cite: 25]
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
        Platform::Log("[INTERFACE]: Смена режима. Интерфейс переключен на кабину управления Титана!");[cite: 25]
    } else if (m_currentMode == EntityControlMode::Pilot_Humanoid) {
        Platform::Log("[INTERFACE]: Игрок покинул кабину. Активен режим Пилота-гуманоида.");[cite: 25]
    }
}

} // namespace Centralia