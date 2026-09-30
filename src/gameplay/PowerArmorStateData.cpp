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
// SECTION 1: DETAILED EXOSKELETON MODEL PROFILES & INITIALIZATION
// ============================================================================

void InitializePowerArmorState(PowerArmorStateData& outState, float initialCharge, float drainModifier) noexcept {
    std::memset(&outState, 0, sizeof(PowerArmorStateData));

    outState.fusionCoreCharge  = std::clamp(initialCharge, 0.0f, 100.0f);
    outState.coreDrainModifier = std::max(0.1f, drainModifier);
    outState.isCoreDepleted    = (outState.fusionCoreCharge <= 0.0f) ? 1 : 0;
    outState.padding           = 0;

    // Инициализация стандартных базовых характеристик 6 узлов (T-60 Standard Spec)
    for (size_t i = 0; i < 6; ++i) {
        ArmorComponent& comp = outState.components[i];
        comp.maxDurability     = 350.0f;
        comp.durability        = 350.0f;
        comp.isBroken          = 0;

        switch (static_cast<ArmorComponentID>(i)) {
            case ArmorComponentID::Torso:
                comp.damageResistance    = 85.0f;
                comp.radiationResistance = 50.0f;
                break;
            case ArmorComponentID::Helmet:
                comp.damageResistance    = 55.0f;
                comp.radiationResistance = 30.0f;
                break;
            case ArmorComponentID::LeftArm:
            case ArmorComponentID::RightArm:
                comp.damageResistance    = 45.0f;
                comp.radiationResistance = 25.0f;
                break;
            case ArmorComponentID::LeftLeg:
            case ArmorComponentID::RightLeg:
                comp.damageResistance    = 50.0f;
                comp.radiationResistance = 30.0f;
                break;
        }
    }

    Platform::Log("[POWER ARMOR STATE DATA]: Standard T-60 state structure successfully allocated and initialized in RAM.");
}

void InitializeAdvancedPowerArmorModel(PowerArmorStateData& outState, PowerArmorModelType modelType) noexcept {
    std::memset(&outState, 0, sizeof(PowerArmorStateData));

    outState.fusionCoreCharge  = 100.0f;
    outState.coreDrainModifier = 1.0f;
    outState.isCoreDepleted    = 0;
    outState.padding           = 0;

    float baseDurability = 300.0f;
    float baseDr = 40.0f;
    float baseRad = 20.0f;

    switch (modelType) {
        case PowerArmorModelType::T_45d:
            baseDurability = 250.0f; baseDr = 50.0f; baseRad = 30.0f;
            outState.coreDrainModifier = 1.2f;
            Platform::Log("[MODEL INIT]: Configured T-45d Heavy Infantry Exoskeleton profile.");
            break;
        case PowerArmorModelType::T_51b:
            baseDurability = 400.0f; baseDr = 90.0f; baseRad = 55.0f;
            outState.coreDrainModifier = 0.9f;
            Platform::Log("[MODEL INIT]: Configured T-51b Polymer-Plated Exoskeleton profile.");
            break;
        case PowerArmorModelType::T_60:
            baseDurability = 450.0f; baseDr = 100.0f; baseRad = 60.0f;
            outState.coreDrainModifier = 1.0f;
            Platform::Log("[MODEL INIT]: Configured T-60 Brotherhood Standard Exoskeleton profile.");
            break;
        case PowerArmorModelType::X_01:
            baseDurability = 550.0f; baseDr = 130.0f; baseRad = 85.0f;
            outState.coreDrainModifier = 0.8f;
            Platform::Log("[MODEL INIT]: Configured X-01 Enclave Advanced Prototype Exoskeleton profile.");
            break;
        case PowerArmorModelType::Raider_Scrap:
            baseDurability = 150.0f; baseDr = 25.0f; baseRad = 10.0f;
            outState.coreDrainModifier = 1.5f;
            Platform::Log("[MODEL INIT]: Configured makeshift Raider Scrap Exoskeleton profile.");
            break;
    }

    for (size_t i = 0; i < 6; ++i) {
        ArmorComponent& comp = outState.components[i];
        comp.maxDurability = baseDurability;
        comp.durability    = baseDurability;
        comp.isBroken      = 0;

        if (i == static_cast<size_t>(ArmorComponentID::Torso)) {
            comp.damageResistance    = baseDr * 1.3f;
            comp.radiationResistance = baseRad * 1.2f;
        } else {
            comp.damageResistance    = baseDr;
            comp.radiationResistance = baseRad;
        }
    }
}

// ============================================================================
// SECTION 2: STATE VALIDATION, INTEGRITY CHECKS & ERROR LOGGING
// ============================================================================

bool ValidatePowerArmorState(const PowerArmorStateData& stateData) noexcept {
    if (stateData.fusionCoreCharge < 0.0f || stateData.fusionCoreCharge > 100.0f) {
        Platform::Log("[STATE VALIDATION ERROR]: Fusion core charge parameter out of bounds [0.0 - 100.0].");
        return false;
    }

    if (stateData.coreDrainModifier <= 0.0f || stateData.coreDrainModifier > 15.0f) {
        Platform::Log("[STATE VALIDATION ERROR]: Core drain modifier corrupted or exceeds maximum limits.");
        return false;
    }

    if (stateData.isCoreDepleted != 0 && stateData.isCoreDepleted != 1) {
        Platform::Log("[STATE VALIDATION ERROR]: Depletion flag corrupted (expected binary 0 or 1).");
        return false;
    }

    for (size_t i = 0; i < 6; ++i) {
        const ArmorComponent& comp = stateData.components[i];
        
        if (comp.durability < 0.0f || comp.durability > comp.maxDurability) {
            Platform::Log("[STATE VALIDATION ERROR]: Component ID " + std::to_string(i) + " durability mismatch.");
            return false;
        }

        if (comp.maxDurability <= 0.0f || comp.maxDurability > 5000.0f) {
            Platform::Log("[STATE VALIDATION ERROR]: Component ID " + std::to_string(i) + " max durability out of bounds.");
            return false;
        }

        if (comp.isBroken != 0 && comp.isBroken != 1) {
            Platform::Log("[STATE VALIDATION ERROR]: Component ID " + std::to_string(i) + " broken flag corrupted.");
            return false;
        }
    }

    return true;
}

// ============================================================================
// SECTION 3: INTEGRITY UTILITIES, METRICS AND STRING DESCRIPTORS
// ============================================================================

float CalculateAverageArmorIntegrity(const PowerArmorStateData& stateData) noexcept {
    float cumulativeRatio = 0.0f;
    size_t validComponentsCount = 0;

    for (size_t i = 0; i < 6; ++i) {
        const ArmorComponent& comp = stateData.components[i];
        if (comp.maxDurability > 0.0f) {
            float ratio = std::clamp(comp.durability / comp.maxDurability, 0.0f, 1.0f);
            cumulativeRatio += ratio;
            validComponentsCount++;
        }
    }

    if (validComponentsCount == 0) return 0.0f;
    return (cumulativeRatio / static_cast<float>(validComponentsCount)) * 100.0f;
}

uint32_t CountBrokenComponents(const PowerArmorStateData& stateData) noexcept {
    uint32_t brokenCount = 0;
    for (size_t i = 0; i < 6; ++i) {
        if (stateData.components[i].isBroken != 0 || stateData.components[i].durability <= 0.0f) {
            brokenCount++;
        }
    }
    return brokenCount;
}

const char* GetArmorComponentName(ArmorComponentID compId) noexcept {
    switch (compId) {
        case ArmorComponentID::Torso:    return "Torso Armor Segment (Торс)";
        case ArmorComponentID::Helmet:   return "Helmet Sensor Unit (Шлем)";
        case ArmorComponentID::LeftArm:  return "Left Arm Manipulator (Левая рука)";
        case ArmorComponentID::RightArm: return "Right Arm Manipulator (Правая рука)";
        case ArmorComponentID::LeftLeg:  return "Left Leg Servo-Actuator (Левая нога)";
        case ArmorComponentID::RightLeg: return "Right Leg Servo-Actuator (Правая нога)";
        default:                         return "Unknown Exoskeleton Component (Неизвестно)";
    }
}

// ============================================================================
// SECTION 4: BINARY PASSPORT PACKING & UNPACKING UTILITIES
// ============================================================================

std::vector<uint8_t> PackStateToBinaryStream(const PowerArmorStateData& stateData) {
    std::vector<uint8_t> buffer;
    buffer.reserve(sizeof(PowerArmorStateData));

    // Сериализация заряда батареи
    const uint8_t* chargePtr = reinterpret_cast<const uint8_t*>(&stateData.fusionCoreCharge);
    buffer.insert(buffer.end(), chargePtr, chargePtr + sizeof(float));

    // Сериализация флагов
    buffer.push_back(stateData.isCoreDepleted);
    buffer.push_back(stateData.padding);

    // Сериализация модификатора
    const uint8_t* modPtr = reinterpret_cast<const uint8_t*>(&stateData.coreDrainModifier);
    buffer.insert(buffer.end(), modPtr, modPtr + sizeof(float));

    // Сериализация 6 компонентов брони
    for (size_t i = 0; i < 6; ++i) {
        const ArmorComponent& comp = stateData.components[i];
        const uint8_t* compPtr = reinterpret_cast<const uint8_t*>(&comp);
        buffer.insert(buffer.end(), compPtr, compPtr + sizeof(ArmorComponent));
    }

    Platform::Log("[STATE PASSPORT]: Power armor state successfully packed into binary stream (" + std::to_string(buffer.size()) + " bytes).");
    return buffer;
}

bool UnpackStateFromBinaryStream(const std::vector<uint8_t>& buffer, PowerArmorStateData& outState) noexcept {
    if (buffer.size() < sizeof(PowerArmorStateData)) {
        Platform::Log("[STATE PASSPORT ERROR]: Binary buffer size too small to unpack power armor state.");
        return false;
    }

    size_t cursor = 0;

    std::memcpy(&outState.fusionCoreCharge, buffer.data() + cursor, sizeof(float));
    cursor += sizeof(float);

    outState.isCoreDepleted = buffer[cursor++];
    outState.padding = buffer[cursor++];

    std::memcpy(&outState.coreDrainModifier, buffer.data() + cursor, sizeof(float));
    cursor += sizeof(float);

    for (size_t i = 0; i < 6; ++i) {
        std::memcpy(&outState.components[i], buffer.data() + cursor, sizeof(ArmorComponent));
        cursor += sizeof(ArmorComponent);
    }

    if (!ValidatePowerArmorState(outState)) {
        Platform::Log("[STATE PASSPORT ERROR]: Unpacked state failed validation checks.");
        return false;
    }

    Platform::Log("[STATE PASSPORT]: Power armor state successfully unpacked and verified from binary stream.");
    return true;
}

} // namespace Centralia_Project_Passport