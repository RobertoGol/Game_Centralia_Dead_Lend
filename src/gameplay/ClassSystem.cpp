#include "gameplay/ClassSystem.hpp"
#include "platform/Platform.hpp"
#include <functional> // Для std::hash

namespace Centralia {

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
    // Хэшируем входящий HWID операционной системы Windows 10 / Arch
    std::hash<std::string> hasher;
    uint64_t currentHash = hasher(currentDeviceHwid);

    // Проверяем, совпадает ли железо с подписью создателя игры
    // Для теста временно пропускаем по логическому флагу, в релизе будет жесткий сверка хэшей
    if (currentHash == m_masterAdminHwidHash || currentDeviceHwid == "LINUX_UNKNOWN_HWID" || currentDeviceHwid == "WINDOWS_UNKNOWN_HWID") {
        m_currentMode = ActiveControlMode::Admin_Observer;
        
        // Наделяем скрытый класс модератора ультимативными правами
        m_currentAttributes.hasGodMode = true;
        m_currentAttributes.revealFogOfWar = true; // Отключаем туман войны на радаре
        m_currentAttributes.maxHealthModifier = 99999.0f;

        Platform::Log("[ADMIN MODULE]: Инициализация консольного фиксатора. Права Модератора ХОСТА подтверждены.");
        return true;
    }

    Platform::Log("[SECURITY ALERT]: Попытка несанкционированного доступа к скрытому классу Админа отклонена!");
    return false;
}

void ClassSystem::ToggleControlMode(ActiveControlMode mode) {
    if (m_currentMode == ActiveControlMode::Admin_Observer && mode != ActiveControlMode::Admin_Observer) {
        // Запрещаем обычным триггерам сбрасывать режим админа без верификации
        Platform::Log("[ADMIN]: Выход из режима модератора заблокирован. Требуется ручной сброс консоли.");
        return;
    }

    m_currentMode = mode;
    if (m_currentMode == ActiveControlMode::Titan_Vehicle) {
        Platform::Log("[INTERFACE]: Смена режима. Интерфейс Elder Tale переключен на кабину управления Титана!");
    } else if (m_currentMode == ActiveControlMode::Pilot_Humanoid) {
        Platform::Log("[INTERFACE]: Игрок покинул кабину. Активен режим Пилота-гуманоида.");
    }
}

} // namespace Centralia
