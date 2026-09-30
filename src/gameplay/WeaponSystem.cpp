#include "gameplay/WeaponSystem.hpp"
#include "gameplay/Player.hpp"
#include "platform/Platform.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <iostream>

namespace Centralia {

WeaponSystem::WeaponSystem() {
    // Инициализация по умолчанию: выдаем базовое оружие выжившего (Ржавый автомат)
    EquipWeapon(101);
}

WeaponSystem::~WeaponSystem() {
    m_activeWeapon = WeaponData{};
}

void WeaponSystem::EquipWeapon(uint32_t weaponId) {
    m_activeWeapon.id = weaponId;
    m_activeWeapon.isReloading = false;
    m_activeWeapon.reloadProgressTimer = 0.0f;

    switch (weaponId) {
        case 101: { // Ржавый автомат 5.45 (Классический магазин)
            m_activeWeapon.name = "Ржавый автомат 5.45 'Пустошь'";
            m_activeWeapon.magType = MagazineType::Magazine_Clip;
            m_activeWeapon.requiredAmmoId = 601; // ID патронов 5.45 мм
            m_activeWeapon.clipMaxCapacity = 30;
            m_activeWeapon.currentAmmoInClip = 30;
            m_activeWeapon.baseDamage = 25.0f;
            m_activeWeapon.fireRateRpm = 600.0f;
            m_activeWeapon.verticalRecoil = 0.45f;
            m_activeWeapon.reloadTimeSec = 2.4f;
            break;
        }
        case 777: { // Револьверный карабин 'Элдер' (Барабан с поштучной зарядкой)
            m_activeWeapon.name = "Револьверный карабин 'Элдер'";
            m_activeWeapon.magType = MagazineType::Revolver_Cylinder;
            m_activeWeapon.requiredAmmoId = 604; // Револьверные патроны .44
            m_activeWeapon.clipMaxCapacity = 6;
            m_activeWeapon.currentAmmoInClip = 6;
            m_activeWeapon.baseDamage = 75.0f;
            m_activeWeapon.fireRateRpm = 120.0f;
            m_activeWeapon.verticalRecoil = 1.35f;
            m_activeWeapon.reloadTimeSec = 0.75f; // Время досыла ОДНОГО патрона в камору
            break;
        }
        case 888: { // Пороховой мушкет выжившего (Дульное заряжание)
            m_activeWeapon.name = "Пороховой мушкет выжившего";
            m_activeWeapon.magType = MagazineType::Powder_Single_Shot;
            m_activeWeapon.requiredAmmoId = 603; // Круглая свинцовая пуля + порох
            m_activeWeapon.clipMaxCapacity = 1;
            m_activeWeapon.currentAmmoInClip = 1;
            m_activeWeapon.baseDamage = 135.0f;
            m_activeWeapon.fireRateRpm = 15.0f;
            m_activeWeapon.verticalRecoil = 3.2f;
            m_activeWeapon.reloadTimeSec = 5.5f;
            break;
        }
        case 999: { // Тяжелое противотанковое орудие Титана 'Молот'
            m_activeWeapon.name = "Противотанковое орудие 'Молот'";
            m_activeWeapon.magType = MagazineType::Single_Shot;
            m_activeWeapon.requiredAmmoId = 602; // Тяжелый бронебойный снаряд
            m_activeWeapon.clipMaxCapacity = 1;
            m_activeWeapon.currentAmmoInClip = 1;
            m_activeWeapon.baseDamage = 550.0f;
            m_activeWeapon.fireRateRpm = 20.0f;
            m_activeWeapon.verticalRecoil = 5.0f;
            m_activeWeapon.reloadTimeSec = 3.8f;
            break;
        }
        default: {
            m_activeWeapon.name = "Стандартный пистолет";
            m_activeWeapon.magType = MagazineType::Magazine_Clip;
            m_activeWeapon.requiredAmmoId = 600;
            m_activeWeapon.clipMaxCapacity = 12;
            m_activeWeapon.currentAmmoInClip = 12;
            m_activeWeapon.baseDamage = 18.0f;
            m_activeWeapon.fireRateRpm = 350.0f;
            m_activeWeapon.verticalRecoil = 0.25f;
            m_activeWeapon.reloadTimeSec = 1.8f;
            break;
        }
    }
    Platform::Log("[WEAPON SYSTEM]: Экипировано оружие -> " + m_activeWeapon.name + " [Емкость: " + std::to_string(m_activeWeapon.clipMaxCapacity) + "]");
}

bool WeaponSystem::Fire(float deltaTime, float playerMovementSpeed, float& outCameraRecoilY) {
    outCameraRecoilY = 0.0f;

    // Проверка состояния перезарядки
    if (m_activeWeapon.isReloading) {
        // Уникальная механика револьвера: прерывание перезарядки кликом мыши, если в барабане есть патроны
        if (m_activeWeapon.magType == MagazineType::Revolver_Cylinder && m_activeWeapon.currentAmmoInClip > 0) {
            m_activeWeapon.isReloading = false;
            Platform::Log("[COMBAT]: Перезарядка барабана прервана игроком! Произведен экстренный выстрел.");
        } else {
            return false; // Обычное оружие не стреляет во время смены магазина
        }
    }

    // Проверка наличия патронов в магазине
    if (m_activeWeapon.currentAmmoInClip <= 0) {
        Platform::Log("[COMBAT]: Щелчок! Магазин пуст. Требуется нажать 'R' для перезарядки.");
        return false;
    }

    // Расходуем боеприпас из магазина
    m_activeWeapon.currentAmmoInClip--;

    // Расчет баллистического разброса с учетом скорости передвижения игрока и веса силовой брони
    float spreadModifier = 1.0f + (playerMovementSpeed * 0.35f);
    
    // Передаем импульс отдачи для подъема 3D-камеры игрока на CPU
    outCameraRecoilY = m_activeWeapon.verticalRecoil * spreadModifier;

    Platform::Log("[FIRE]: Выстрел из пушки '" + m_activeWeapon.name + "'. Остаток в магазине: " + 
                  std::to_string(m_activeWeapon.currentAmmoInClip) + "/" + std::to_string(m_activeWeapon.clipMaxCapacity));

    return true;
}

void WeaponSystem::StartReload(Player& player) {
    if (m_activeWeapon.isReloading) return;
    if (m_activeWeapon.currentAmmoInClip >= m_activeWeapon.clipMaxCapacity) {
        Platform::Log("[RELOAD]: Магазин уже полностью полон.");
        return;
    }

    // Проверяем, есть ли вообще нужные патроны в инвентаре игрока перед стартом анимации
    bool ammoFound = false;
    const auto& inventory = player.GetInventory();
    for (const auto& item : inventory) {
        if (item.id == m_activeWeapon.requiredAmmoId && item.quantity > 0) {
            ammoFound = true;
            break;
        }
    }

    if (!ammoFound) {
        Platform::Log("[RELOAD ERROR]: В инвентаре отсутствуют патроны типа ID: " + std::to_string(m_activeWeapon.requiredAmmoId));
        return;
    }

    m_activeWeapon.isReloading = true;
    m_activeWeapon.reloadProgressTimer = 0.0f;
    Platform::Log("[COMBAT]: Начата перезарядка оружия: " + m_activeWeapon.name);
}

void WeaponSystem::UpdateWeaponTick(float deltaTime, Player& player) {
    if (!m_activeWeapon.isReloading) return;

    m_activeWeapon.reloadProgressTimer += deltaTime;

    // 1. РЕВОЛЬВЕРНЫЙ ТИП ЗАРЯДКИ (Поштучное помещение патронов в каморы барабана)
    if (m_activeWeapon.magType == MagazineType::Revolver_Cylinder) {
        if (m_activeWeapon.reloadProgressTimer >= m_activeWeapon.reloadTimeSec) {
            m_activeWeapon.reloadProgressTimer = 0.0f;

            // Ищем патрон в инвентаре и списываем ровно 1 штуку
            bool deducted = false;
            const auto& inv = player.GetInventory();
            for (size_t i = 0; i < inv.size(); ++i) {
                if (inv[i].id == m_activeWeapon.requiredAmmoId && inv[i].quantity > 0) {
                    player.RemoveItem(i, 1);
                    deducted = true;
                    break;
                }
            }

            if (!deducted) {
                m_activeWeapon.isReloading = false;
                Platform::Log("[RELOAD WARNING]: Патроны в инвентаре закончились! Зарядка барабана прервана.");
                return;
            }

            m_activeWeapon.currentAmmoInClip++;
            Platform::Log("[RELOAD]: Патрон успешно закинут в камору барабана [" + std::to_string(m_activeWeapon.currentAmmoInClip) + "/" + std::to_string(m_activeWeapon.clipMaxCapacity) + "]");

            // Если барабан полон — завершаем процесс перезарядки
            if (m_activeWeapon.currentAmmoInClip >= m_activeWeapon.clipMaxCapacity) {
                m_activeWeapon.isReloading = false;
                Platform::Log("[RELOAD]: Барабан полностью снаряжен.");
            }
        }
    }
    // 2. СТАНДАРТНЫЙ ТИП (Мгновенная замена магазина целиком по истечении таймера)
    else {
        if (m_activeWeapon.reloadProgressTimer >= m_activeWeapon.reloadTimeSec) {
            uint16_t neededAmmo = m_activeWeapon.clipMaxCapacity - m_activeWeapon.currentAmmoInClip;
            uint16_t loadedTotal = 0;

            const auto& inv = player.GetInventory();
            for (size_t i = 0; i < inv.size(); ++i) {
                if (inv[i].id == m_activeWeapon.requiredAmmoId) {
                    uint16_t takeCount = std::min(neededAmmo, inv[i].quantity);
                    player.RemoveItem(i, takeCount);
                    loadedTotal += takeCount;
                    neededAmmo -= takeCount;
                    if (neededAmmo == 0) break;
                }
            }

            m_activeWeapon.isReloading = false;

            if (loadedTotal == 0) {
                Platform::Log("[RELOAD ERROR]: Не удалось извлечь патроны из инвентаря.");
                return;
            }

            m_activeWeapon.currentAmmoInClip += loadedTotal;
            Platform::Log("[RELOAD]: Магазин заменен. Заряжено патронов: " + std::to_string(m_activeWeapon.currentAmmoInClip));
        }
    }
}

} // namespace Centralia