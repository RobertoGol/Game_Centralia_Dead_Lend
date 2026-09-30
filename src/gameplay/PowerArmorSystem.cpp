#include "gameplay/PowerArmorStateData.hpp"
#include "core/MemoryManager.hpp"
#include "platform/Platform.hpp"
#include <cstring>
#include <algorithm>
#include <string>
#include <vector>
#include <cmath>
#include <iostream>

namespace Centralia_Project_Passport {

// ============================================================================
// SECTION 1: CONSTRUCTORS, DESTRUCTORS & STATE RESET MECHANICS
// ============================================================================

PowerArmorEngineContext::PowerArmorEngineContext() noexcept 
    : m_totalRuntimeSeconds(0.0),
      m_servoOverloadActive(false),
      m_internalTemperature(36.5f),
      m_radiationShieldingEfficiency(1.0f),
      m_jetpackActive(false),
      m_jetpackFuel(100.0f),
      m_nightVisionEnabled(false),
      m_diagnosticErrorCount(0)
{
    ResetToDefault();
    Platform::Log("[POWER ARMOR SYSTEM CONSTRUCTOR]: Advanced T-60 / X-01 exoskeleton context allocated with full diagnostic telemetry.");
}

PowerArmorEngineContext::~PowerArmorEngineContext() noexcept {
    ShutdownExoskeleton();
}

void PowerArmorEngineContext::ResetToDefault() noexcept {
    std::memset(&m_State, 0, sizeof(PowerArmorStateData));
    
    m_State.fusionCoreCharge    = 100.0f;
    m_State.coreDrainModifier   = 1.0f; 
    m_State.isCoreDepleted      = 0;
    m_State.padding             = 0;

    // Инициализация покомпонентной прочности 6 узлов силовой брони (T-60 Specification)
    for (size_t i = 0; i < 6; ++i) {
        ArmorComponent& comp = m_State.components[i];
        comp.maxDurability = 400.0f;
        comp.durability    = 400.0f;
        comp.isBroken      = 0;
        
        switch (static_cast<ArmorComponentID>(i)) {
            case ArmorComponentID::Torso:
                comp.damageResistance = 95.0f;
                comp.radiationResistance = 60.0f;
                break;
            case ArmorComponentID::Helmet:
                comp.damageResistance = 65.0f;
                comp.radiationResistance = 35.0f;
                break;
            case ArmorComponentID::LeftArm:
            case ArmorComponentID::RightArm:
                comp.damageResistance = 50.0f;
                comp.radiationResistance = 25.0f;
                break;
            case ArmorComponentID::LeftLeg:
            case ArmorComponentID::RightLeg:
                comp.damageResistance = 60.0f;
                comp.radiationResistance = 30.0f;
                break;
        }
    }

    m_totalRuntimeSeconds = 0.0;
    m_servoOverloadActive = false;
    m_internalTemperature = 37.0f;
    m_radiationShieldingEfficiency = 1.0f;
    m_jetpackActive = false;
    m_jetpackFuel = 100.0f;
    m_nightVisionEnabled = false;
    m_diagnosticErrorCount = 0;

    Platform::Log("[POWER ARMOR RESET]: Exoskeleton telemetry and diagnostic registers successfully reverted to factory-fresh baseline.");
}

void PowerArmorEngineContext::ShutdownExoskeleton() noexcept {
    std::memset(&m_State, 0, sizeof(PowerArmorStateData));
    m_jetpackActive = false;
    m_nightVisionEnabled = false;
    Platform::Log("[POWER ARMOR SHUTDOWN]: Hydraulic pressure vented completely. Servo-motors powered down safely. Telemetry unmapped.");
}

// ============================================================================
// SECTION 2: POWER GRID, FUSION CORE DRAIN, AND SERVO-ACTUATORS
// ============================================================================

void PowerArmorEngineContext::ProcessPowerGridTick(float deltaTime, bool isShiftPressed, float& outLinearVelocity) noexcept {
    m_totalRuntimeSeconds += static_cast<double>(deltaTime);

    if (m_State.isCoreDepleted) {
        outLinearVelocity *= 0.2f;
        m_servoOverloadActive = false;
        m_jetpackActive = false;
        m_nightVisionEnabled = false;
        return;
    }

    float drainRate = CONST_IDLE_DRAIN;

    // Дополнительное энергопотребление при ночном видении
    if (m_nightVisionEnabled) {
        drainRate += 0.05f;
    }

    // Реактивные ускорители (Jetpack)
    if (m_jetpackActive && m_jetpackFuel > 0.0f) {
        drainRate += 1.8f;
        m_jetpackFuel -= 15.0f * deltaTime;
        if (m_jetpackFuel <= 0.0f) {
            m_jetpackFuel = 0.0f;
            m_jetpackActive = false;
            Platform::Log("[JETPACK WARNING]: Thruster fuel depleted! Forced descent initiated.");
        }
    } else {
        // Медленное восстановление топлива в покое на земле
        if (m_jetpackFuel < 100.0f) {
            m_jetpackFuel += 5.0f * deltaTime;
            if (m_jetpackFuel > 100.0f) m_jetpackFuel = 100.0f;
        }
    }

    if (isShiftPressed && outLinearVelocity > 0.1f) {
        drainRate += CONST_SPRINT_DRAIN;
        outLinearVelocity *= 1.7f; 
        
        m_internalTemperature += 0.6f * deltaTime;
        if (m_internalTemperature > 90.0f) {
            m_servoOverloadActive = true;
            m_diagnosticErrorCount++;
            Platform::Log("[POWER ARMOR THERMAL WARNING [ERR-04]]: Reactor core overheating critically! Servo efficiency throttled.");
        }
    } else {
        if (m_internalTemperature > 37.0f) {
            m_internalTemperature -= 1.0f * deltaTime;
            if (m_internalTemperature <= 37.0f) {
                m_internalTemperature = 37.0f;
                m_servoOverloadActive = false;
            }
        }
    }

    float finalDrainFactor = drainRate * m_State.coreDrainModifier;
    if (m_servoOverloadActive) {
        finalDrainFactor *= 1.5f; 
    }

    m_State.fusionCoreCharge -= finalDrainFactor * deltaTime;

    if (m_State.fusionCoreCharge <= 0.0f) {
        m_State.fusionCoreCharge = 0.0f;
        m_State.isCoreDepleted   = 1;
        outLinearVelocity       *= 0.2f;
        m_jetpackActive          = false;
        m_nightVisionEnabled     = false;
        Platform::Log("[POWER ARMOR FATAL [ERR-99]]: Fusion core absolute energy exhaustion. Hydraulics completely locked!");
    }
}

void PowerArmorEngineContext::RegisterHeavyCombatAction() noexcept {
    if (m_State.isCoreDepleted) return;

    m_State.fusionCoreCharge -= CONST_ACTION_DRAIN;
    m_internalTemperature += 1.5f; 

    if (m_State.fusionCoreCharge <= 0.0f) {
        m_State.fusionCoreCharge = 0.0f;
        m_State.isCoreDepleted   = 1;
        Platform::Log("[POWER ARMOR WARNING]: High-intensity combat load completely drained fusion cell reserves.");
    }
}

void PowerArmorEngineContext::HotSwapFusionCore() noexcept {
    m_State.fusionCoreCharge = 100.0f;
    m_State.isCoreDepleted   = 0;
    m_internalTemperature = 39.0f;
    m_servoOverloadActive = false;
    m_diagnosticErrorCount = 0;
    Platform::Log("[POWER ARMOR CORE SWAP]: Spent fusion cell ejected. Fresh 100% nuclear core locked in place. Systems nominal.");
}

// ============================================================================
// SECTION 3: ADVANCED COMPONENT DAMAGE, ARMOR MITIGATION & WEAR
// ============================================================================

void PowerArmorEngineContext::ComputeComponentDamage(ArmorComponentID targetComp, float& ioDamage) noexcept {
    uint8_t index = static_cast<uint8_t>(targetComp);
    if (index >= 6) return;

    ArmorComponent& comp = m_State.components[index];

    if (comp.isBroken) {
        return; // Уничтоженная пластина пропускает урон напрямую оператору
    }

    float mitigation = comp.damageResistance;
    if (mitigation > ioDamage * 0.90f) {
        mitigation = ioDamage * 0.90f; 
    }

    ioDamage -= mitigation;
    
    float wearFactor = mitigation * 0.4f;
    comp.durability -= wearFactor;

    if (comp.durability <= 0.0f) {
        comp.durability = 0.0f;
        comp.isBroken   = 1;
        m_diagnosticErrorCount++;
        Platform::Log("[POWER ARMOR PLATE SHATTERED]: Armor segment ID [" + std::to_string(index) + "] structural failure!");
    }
}

void PowerArmorEngineContext::RepairComponent(ArmorComponentID targetComp, float repairAmount) noexcept {
    uint8_t index = static_cast<uint8_t>(targetComp);
    if (index >= 6) return;

    ArmorComponent& comp = m_State.components[index];
    comp.durability += repairAmount;
    
    if (comp.durability > comp.maxDurability) {
        comp.durability = comp.maxDurability;
    }

    if (comp.durability > 0.0f && comp.isBroken) {
        comp.isBroken = 0;
        if (m_diagnosticErrorCount > 0) m_diagnosticErrorCount--;
    }

    Platform::Log("[POWER ARMOR FIELD REPAIR]: Component [" + std::to_string(index) + "] field welded. Durability restored to: " + std::to_string(comp.durability));
}

// ============================================================================
// SECTION 4: JETPACK, NIGHT VISION AND HUD TELEMETRY CONTROLS
// ============================================================================

void PowerArmorEngineContext::ToggleJetpack(bool enable) noexcept {
    if (m_State.isCoreDepleted) {
        m_jetpackActive = false;
        return;
    }
    m_jetpackActive = enable;
    Platform::Log(enable ? "[JETPACK]: Thrust vector control engaged." : "[JETPACK]: Thrusters disengaged.");
}

void PowerArmorEngineContext::ToggleNightVision(bool enable) noexcept {
    if (m_State.isCoreDepleted) {
        m_nightVisionEnabled = false;
        return;
    }
    m_nightVisionEnabled = enable;
    Platform::Log(enable ? "[OPTICS]: HUD tactical night-vision online." : "[OPTICS]: Tactical night-vision offline.");
}

// ============================================================================
// SECTION 5: TELEMETRY, CARRY WEIGHT, AND STATUS QUERY API
// ============================================================================

float PowerArmorEngineContext::GetTotalDamageResistance() const noexcept {
    float totalDr = 0.0f;
    for (size_t i = 0; i < 6; ++i) {
        if (!m_State.components[i].isBroken) {
            totalDr += m_State.components[i].damageResistance * (m_State.components[i].durability / m_State.components[i].maxDurability);
        }
    }
    return totalDr;
}

float PowerArmorEngineContext::GetTotalRadiationResistance() const noexcept {
    float totalRad = 0.0f;
    for (size_t i = 0; i < 6; ++i) {
        if (!m_State.components[i].isBroken) {
            totalRad += m_State.components[i].radiationResistance;
        }
    }
    return totalRad * m_radiationShieldingEfficiency;
}

float PowerArmorEngineContext::GetCarryWeightBonus() const noexcept {
    if (m_State.isCoreDepleted) {
        return 30.0f; 
    }
    return 300.0f; // Увеличенный бонус грузоподъемности экзоскелета
}

bool PowerArmorEngineContext::IsOperational() const noexcept {
    return !m_State.isCoreDepleted;
}

uint32_t PowerArmorEngineContext::GetDiagnosticErrorCount() const noexcept {
    return m_diagnosticErrorCount;
}

// ============================================================================
// SECTION 6: BINARY SERIALIZATION FOR SAVEGAMES (GHOST-RAM / DISK)
// ============================================================================

std::vector<uint8_t> PowerArmorEngineContext::SerializeToBinary() const {
    std::vector<uint8_t> stream;
    stream.reserve(sizeof(PowerArmorStateData) + 32);

    // Упаковываем заряд батареи
    const uint8_t* chargePtr = reinterpret_cast<const uint8_t*>(&m_State.fusionCoreCharge);
    stream.insert(stream.end(), chargePtr, chargePtr + sizeof(float));

    // Упаковываем флаги истощения
    stream.push_back(m_State.isCoreDepleted);
    stream.push_back(m_State.padding);

    // Упаковываем множитель
    const uint8_t* modPtr = reinterpret_cast<const uint8_t*>(&m_State.coreDrainModifier);
    stream.insert(stream.end(), modPtr, modPtr + sizeof(float));

    // Упаковываем все 6 компонентов брони
    for (size_t i = 0; i < 6; ++i) {
        const ArmorComponent& comp = m_State.components[i];
        const uint8_t* compPtr = reinterpret_cast<const uint8_t*>(&comp);
        stream.insert(stream.end(), compPtr, compPtr + sizeof(ArmorComponent));
    }

    // Дополнительные параметры телеметрии для сохранения
    const uint8_t* fuelPtr = reinterpret_cast<const uint8_t*>(&m_jetpackFuel);
    stream.insert(stream.end(), fuelPtr, fuelPtr + sizeof(float));

    const uint8_t* tempPtr = reinterpret_cast<const uint8_t*>(&m_internalTemperature);
    stream.insert(stream.end(), tempPtr, tempPtr + sizeof(float));

    return stream;
}

bool PowerArmorEngineContext::DeserializeFromBinary(const std::vector<uint8_t>& stream) {
    if (stream.size() < sizeof(PowerArmorStateData)) {
        Platform::Log("[POWER ARMOR ERROR]: Corrupted binary save stream for exoskeleton context.");
        return false;
    }

    size_t offset = 0;
    std::memcpy(&m_State.fusionCoreCharge, stream.data() + offset, sizeof(float));
    offset += sizeof(float);

    m_State.isCoreDepleted = stream[offset++];
    m_State.padding = stream[offset++];

    std::memcpy(&m_State.coreDrainModifier, stream.data() + offset, sizeof(float));
    offset += sizeof(float);

    for (size_t i = 0; i < 6; ++i) {
        std::memcpy(&m_State.components[i], stream.data() + offset, sizeof(ArmorComponent));
        offset += sizeof(ArmorComponent);
    }

    if (offset + sizeof(float) <= stream.size()) {
        std::memcpy(&m_jetpackFuel, stream.data() + offset, sizeof(float));
        offset += sizeof(float);
    }

    if (offset + sizeof(float) <= stream.size()) {
        std::memcpy(&m_internalTemperature, stream.data() + offset, sizeof(float));
        offset += sizeof(float);
    }

    Platform::Log("[POWER ARMOR LOAD]: Exoskeleton telemetry and sub-modules successfully restored from binary dump.");
    return true;
}

} // namespace Centralia_Project_Passport