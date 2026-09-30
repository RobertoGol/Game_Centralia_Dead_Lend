#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "core/Math3D.hpp"
#include "platform/Platform.hpp"

namespace Centralia {

enum class WeaponType : uint8_t {
    Pistol,
    Rifle,
    Shotgun,
    EnergyRifle,
    HeavyPlasma
};

struct WeaponStats {
    uint32_t weaponId;
    std::string weaponName;
    WeaponType type;
    float damage;
    float fireRate;
    uint32_t currentAmmo;
    uint32_t maxMagazineSize;
    bool isReloading;
};

class WeaponsSystem {
private:
    std::vector<WeaponStats> m_inventoryWeapons;
    uint32_t m_activeWeaponIndex;
    bool m_isAiming;

public:
    WeaponsSystem();
    ~WeaponsSystem() = default;

    WeaponsSystem(const WeaponsSystem&) = delete;
    WeaponsSystem& operator=(const WeaponsSystem&) = delete;

    void EquipWeapon(uint32_t weaponId);
    bool FireActiveWeapon();
    void ReloadActiveWeapon();
    void Update(float deltaTime);

    [[nodiscard]] const WeaponStats* GetActiveWeapon() const noexcept;
    [[nodiscard]] bool IsAiming() const noexcept { return m_isAiming; }
    void SetAiming(bool aiming) noexcept { m_isAiming = aiming; }
};

} // namespace Centralia