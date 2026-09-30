#pragma once
#include <string>
#include <cstdint>
#include "platform/Platform.hpp"

namespace Centralia {

enum class PowerArmorModel : uint8_t {
    T_45,
    T_51,
    T_60,
    X_01,
    Hellfire,
    Raider,       // Кустарная рейдерская силовая броня
    Arion         // Экспериментальная модель «Арион»
};

struct PowerArmorStateData {
    PowerArmorModel modelType = PowerArmorModel::T_60;
    float fusionCoreCharge = 100.0f; 
    float hydraulicPressure = 100.0f; 
    float structuralIntegrity = 100.0f; 
    bool isHelmetAttached = true;
    bool isServosActive = false;

    void ResetToDefault() noexcept;
    bool ConsumePower(float amount) noexcept;
    [[nodiscard]] bool IsOperational() const noexcept;
};

class PowerArmorSystem {
private:
    PowerArmorStateData m_currentState;
    bool m_isInArmor;

public:
    PowerArmorSystem();
    ~PowerArmorSystem() = default;

    PowerArmorSystem(const PowerArmorSystem&) = delete;
    PowerArmorSystem& operator=(const PowerArmorSystem&) = delete;

    void EnterArmor(const PowerArmorStateData& data);
    void ExitArmor();
    void Update(float deltaTime);

    [[nodiscard]] const PowerArmorStateData& GetState() const noexcept { return m_currentState; }
    [[nodiscard]] bool IsInArmor() const noexcept { return m_isInArmor; }
};

} // namespace Centralia