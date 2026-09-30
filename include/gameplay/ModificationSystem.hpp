#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "platform/Platform.hpp"

namespace Centralia {

struct VehicleModification {
    uint32_t modId;
    std::string modName;
    std::string category;
    float armorBonus;
    float speedBonus;
    float radResistance;
};

class ModificationsSystem {
private:
    std::vector<VehicleModification> m_installedMods;

public:
    ModificationsSystem() = default;
    ~ModificationsSystem() = default;

    ModificationsSystem(const ModificationsSystem&) = delete;
    ModificationsSystem& operator=(const ModificationsSystem&) = delete;

    bool InstallMod(const VehicleModification& mod);
    bool RemoveMod(uint32_t modId);
    
    [[nodiscard]] float GetTotalArmorBonus() const noexcept;
    [[nodiscard]] float GetTotalSpeedBonus() const noexcept;
    [[nodiscard]] const std::vector<VehicleModification>& GetInstalledMods() const noexcept { return m_installedMods; }
};

} // namespace Centralia