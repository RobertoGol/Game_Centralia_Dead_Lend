#include "gameplay/WeaponSystem.hpp"
#include "gameplay/Player.hpp"
#include "platform/Platform.hpp"
#include <algorithm>

namespace Centralia {

WeaponSystem::WeaponSystem() {
    EquipWeapon(101); // По умолчанию выдаем ржавый автомат
}

void WeaponSystem::EquipWeapon(uint32_t weaponId) {
    m_activeWeapon.id = weaponId;
    m_activeWeapon.isReloading = false;
    m_activeWeapon.reloadProgressTimer = 0.0f;

    // Конфигурируем разные типы магазинов на основе твоих требований
    if (weaponId == 101) { // Наш ржавый автомат из базы
        m_activeWeapon.name = "Ржавый автомат 5.45";
        m_activeWeapon.magType = MagazineType::Magazine_Clip; // Магазинный тип
        m_activeWeapon.requiredAmmoId = 601; // ID патронов 5.45
        m_activeWeapon.clipMaxCapacity = 30;
        m_activeWeapon.currentAmmoInClip = 30;
        m_activeWeapon.baseDamage = 25.0f;
        m_activeWeapon.fireRateRpm = 600.0f;
        m_activeWeapon.verticalRecoil = 0.4f; // Умеренная отдача шутера
        m_activeWeapon.reloadTimeSec = 2.5f;   // Быстро заменил рожок
    } 
    else if (weaponId == 999) { // Тяжелая пушка Титана со скриншота
        m_activeWeapon.name = "Противотанковое орудие 'Молот'";
        m_activeWeapon.magType = MagazineType::Single_Shot; // Однопатронный тип
        m_activeWeapon.requiredAmmoId = 602; // Тяжелый снаряд
        m_activeWeapon.clipMaxCapacity = 1;
        m_activeWeapon.currentAmmoInClip = 1;
        m_activeWeapon.baseDamage = 500.0f;
        m_activeWeapon.fireRateRpm = 12.0f;
        m_activeWeapon.verticalRecoil = 4.5f; // Огромная отдача, шасси трясется
        m_activeWeapon.reloadTimeSec = 4.0f;  
    }
    else if (weaponId == 888) { // Кустарный мушкет Пустоши Centralia
        m_activeWeapon.name = "Пороховой мушкет выжившего";
        m_activeWeapon.magType = MagazineType::Powder_Single_Shot; // Порошково-однопатронный
        m_activeWeapon.requiredAmmoId = 603; // Круглая пуля (нужен еще порох)
        m_activeWeapon.clipMaxCapacity = 1;
        m_activeWeapon.currentAmmoInClip = 0;
        m_activeWeapon.baseDamage = 120.0f;
        m_activeWeapon.fireRateRpm = 6.0f;
        m_activeWeapon.verticalRecoil = 2.0f;
        m_activeWeapon.reloadTimeSec = 6.0f; // Долгая зарядка: насыпать порох, забить пулю
    }
    else if (weaponId == 777) { // Магический револьверный карабин из Log Horizon
        m_activeWeapon.name = "Револьверный карабин 'Элдер'";
        m_activeWeapon.magType = MagazineType::Revolver_Cylinder; // Револьверный тип
        m_activeWeapon.requiredAmmoId = 604;
        m_activeWeapon.clipMaxCapacity = 6; // 6 камор в барабане
        m_activeWeapon.currentAmmoInClip = 6;
        m_activeWeapon.baseDamage = 65.0f;
        m_activeWeapon.fireRateRpm = 120.0f;
        m_activeWeapon.verticalRecoil = 1.2f;
        m_activeWeapon.reloadTimeSec = 0.8f; // Время зарядки ОДНОГО патрона в барабан
    }
}

bool WeaponSystem::Fire(float deltaTime, float playerMovementSpeed, float& outCameraRecoilY) {
    outCameraRecoilY = 0.0f;

    if (m_activeWeapon.isReloading) {
        // Эффект прерывания револьверного типа: если жмем ЛКМ при зарядке барабана, огонь открывается тем, что есть!
        if (m_activeWeapon.magType == MagazineType::Revolver_Cylinder && m_activeWeapon.currentAmmoInClip > 0) {
            m_activeWeapon.isReloading = false;
            Platform::Log("[COMBAT]: Перезарядка револьвера прервана игроком! Открыт огонь.");
        } else {
            return false; // Обычные пушки не стреляют во время перезарядки
        }
    }

    if (m_activeWeapon.currentAmmoInClip == 0) {
        Platform::Log("[COMBAT]: Щелчок! Сухой спуск. Магазин пуст. Требуется перезарядка на кнопку 'R'.");
        return false;
    }

    // Тратим патрон
    m_activeWeapon.currentAmmoInClip--;
    
    // Рассчитываем разлет пуль (spreadFactor) от веса брони и скорости бега Fallout 76
    float spreadModifier = 1.0f + (playerMovementSpeed * 0.5f);
    
    // Передаем импульс отдачи процессору для увода 3D-камеры вверх
    outCameraRecoilY = m_activeWeapon.verticalRecoil * spreadModifier;

    Platform::Log("[FIRE]: Выстрел из '" + m_activeWeapon.name + "'. Патронов в пушке: " + 
                  std::to_string(m_activeWeapon.currentAmmoInClip) + "/" + std::to_string(m_activeWeapon.clipMaxCapacity));

    // Автоматический досыл для однопатронных систем пушек Титанов
    if (m_activeWeapon.currentAmmoInClip == 0 && m_activeWeapon.magType == MagazineType::Single_Shot) {
        Platform::Log("[COMBAT]: Однопатронная пушка ушла в автоматический цикл досыла.");
    }

    return true;
}

void WeaponSystem::StartReload(Player& player) {
    if (m_activeWeapon.isReloading || m_activeWeapon.currentAmmoInClip >= m_activeWeapon.clipMaxCapacity) return;

    Platform::Log("[COMBAT]: Запущена перезарядка оружия. Тип боепитания: " + std::to_string(static_cast<int>(m_activeWeapon.magType)));
    m_activeWeapon.isReloading = true;
    m_activeWeapon.reloadProgressTimer = 0.0f;
}

void WeaponSystem::UpdateWeaponTick(float deltaTime, Player& player) {
    if (!m_activeWeapon.isReloading) return;

    m_activeWeapon.reloadProgressTimer += deltaTime;

    // РЕВОЛЬВЕРНЫЙ ТИП: Патроны заскакивают в барабан поочередно
    if (m_activeWeapon.magType == MagazineType::Revolver_Cylinder) {
        if (m_activeWeapon.reloadProgressTimer >= m_activeWeapon.reloadTimeSec) {
            m_activeWeapon.reloadProgressTimer = 0.0f;
            m_activeWeapon.currentAmmoInClip++;
            Platform::Log("[RELOAD]: Патрон дослан в камору барабана [" + std::to_string(m_activeWeapon.currentAmmoInClip) + "]");
            
            if (m_activeWeapon.currentAmmoInClip >= m_activeWeapon.clipMaxCapacity) {
                m_activeWeapon.isReloading = false;
                Platform::Log("[RELOAD]: Барабан револьверного карабина полностью заполнен.");
            }
        }
    } 
    // ВСЕ ОСТАЛЬНЫЕ ТИПЫ (Мгновенная замена магазина целиком по истечении таймера)
    else {
        if (m_activeWeapon.reloadProgressTimer >= m_activeWeapon.reloadTimeSec) {
            m_activeWeapon.currentAmmoInClip = m_activeWeapon.clipMaxCapacity;
            m_activeWeapon.isReloading = false;
            Platform::Log("[RELOAD]: Перезарядка пушки завершена. Магазин заменен.");
        }
    }
}

} // namespace Centralia
