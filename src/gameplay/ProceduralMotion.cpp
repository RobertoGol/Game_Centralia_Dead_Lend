#include "gameplay/VehiclePhysics.hpp"
#include "platform/Platform.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <iostream>
#include <vector>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTRUCTORS, DESTRUCTORS & CHASSIS INITIALIZATION
// ============================================================================

VehiclePhysics::VehiclePhysics(VehicleType type) 
    : m_vehicleType(type),
      m_baseSpeed(14.0f),
      m_baseArmor(60.0f),
      m_currentThrottle(0.0f),
      m_currentSteering(0.0f),
      m_currentBraking(0.0f),
      m_handbrakeActive(false),
      m_vehicleMass(3200.0f),
      m_currentRpm(1000.0f),
      m_maxRpm(8000.0f),
      m_currentGear(1),
      m_totalGearCount(6),
      m_angularVelocity(0.0f),
      m_cameraShakeIntensity(0.0f),
      m_flightAltitude(0.0f),
      m_suspensionStiffness(40000.0f),
      m_suspensionDamping(4500.0f),
      m_tireGripCoefficient(2.1f)
{
    m_velocity = Vector3D(0.0f, 0.0f, 0.0f);
    m_acceleration = Vector3D(0.0f, 0.0f, 0.0f);
    m_wheels.clear();
    m_legs.clear();
}

VehiclePhysics::~VehiclePhysics() {
    m_wheels.clear();
    m_legs.clear();
    Platform::Log("[VEHICLE PHYSICS DESTROYER]: Chassis subsystem memory cleanly unallocated.");
}

void VehiclePhysics::SetupChassis(const VehicleModification& baseModTemplate) {
    m_wheels.clear();
    m_legs.clear();

    switch (m_vehicleType) {
        case VehicleType::WheeledCar: {
            m_wheels.resize(4);
            for (size_t i = 0; i < 4; ++i) {
                VehicleModification& wheel = m_wheels[i];
                wheel = baseModTemplate;
                wheel.id = static_cast<uint32_t>(baseModTemplate.id + i);
                wheel.health = 200.0f;
                wheel.armorValue = 35.0f;
                wheel.tirePressurePsi = 2.6f;
                wheel.terrainPassability = 1.0f;
                wheel.speedMultiplier = 1.0f;
                wheel.isDetached = false;
            }
            m_vehicleMass = 2100.0f;
            m_baseSpeed = 20.0f;
            Platform::Log("[VEHICLE PHYSICS SETUP]: 4x4 Wheeled Car chassis fully initialized with independent suspension nodes.");
            break;
        }
        case VehicleType::Motorcycle: {
            m_wheels.resize(2);
            for (size_t i = 0; i < 2; ++i) {
                VehicleModification& wheel = m_wheels[i];
                wheel = baseModTemplate;
                wheel.id = static_cast<uint32_t>(baseModTemplate.id + i);
                wheel.health = 120.0f;
                wheel.armorValue = 20.0f;
                wheel.tirePressurePsi = 2.4f;
                wheel.terrainPassability = 0.9f;
                wheel.speedMultiplier = 1.5f;
                wheel.isDetached = false;
            }
            m_vehicleMass = 500.0f;
            m_baseSpeed = 28.0f;
            Platform::Log("[VEHICLE PHYSICS SETUP]: Dual-wheel motorcycle high-speed chassis mapped.");
            break;
        }
        case VehicleType::TrackedTank: {
            m_wheels.resize(2);
            for (size_t i = 0; i < 2; ++i) {
                VehicleModification& wheel = m_wheels[i];
                wheel = baseModTemplate;
                wheel.id = static_cast<uint32_t>(baseModTemplate.id + i);
                wheel.health = 600.0f;
                wheel.armorValue = 120.0f;
                wheel.tirePressurePsi = 0.0f;
                wheel.terrainPassability = 1.6f;
                wheel.speedMultiplier = 0.6f;
                wheel.isDetached = false;
            }
            m_vehicleMass = 11000.0f;
            m_baseSpeed = 9.0f;
            Platform::Log("[VEHICLE PHYSICS SETUP]: Heavy tracked tank assembly locked into RAM.");
            break;
        }
        case VehicleType::TitanHumanoid:
        case VehicleType::TitanMultiLegged: {
            size_t legCount = (m_vehicleType == VehicleType::TitanHumanoid) ? 2 : 4;
            m_legs.resize(legCount);
            for (size_t i = 0; i < legCount; ++i) {
                TitanLegStructure& leg = m_legs[i];
                leg.legId = static_cast<uint32_t>(i);
                leg.health = 800.0f;
                leg.armorValue = 150.0f;
                leg.isCrippled = false;
                leg.isGrounded = true;
                leg.stepProgress = 1.0f;
            }
            m_vehicleMass = 18000.0f;
            m_baseSpeed = 11.0f;
            Platform::Log("[VEHICLE PHYSICS SETUP]: Advanced biped/quad mech titan legs mapped with IK buffers.");
            break;
        }
        case VehicleType::AircraftDrone: {
            m_vehicleMass = 1100.0f;
            m_baseSpeed = 55.0f;
            m_flightAltitude = 60.0f;
            Platform::Log("[VEHICLE PHYSICS SETUP]: Aerial drone / VTOL aerodynamic profile activated.");
            break;
        }
    }
}

// ============================================================================
// SECTION 2: INPUTS, TRANSMISSION, AND ENGINE RPM DYNAMICS
// ============================================================================

void VehiclePhysics::ApplyInputs(float throttle, float steering, float braking, bool handbrake) {
    m_currentThrottle = std::clamp(throttle, -1.0f, 1.0f);
    m_currentSteering = std::clamp(steering, -1.0f, 1.0f);
    m_currentBraking = std::clamp(braking, 0.0f, 1.0f);
    m_handbrakeActive = handbrake;
}

void VehiclePhysics::UpdateTransmission(float deltaTime) {
    if (m_vehicleType == VehicleType::AircraftDrone) return;

    float targetRpm = 1100.0f + (std::abs(m_currentThrottle) * 6900.0f);
    
    if (m_currentRpm < targetRpm) {
        m_currentRpm += 4500.0f * deltaTime;
        if (m_currentRpm > m_maxRpm) m_currentRpm = m_maxRpm;
    } else if (m_currentRpm > targetRpm) {
        m_currentRpm -= 3500.0f * deltaTime;
        if (m_currentRpm < 1100.0f) m_currentRpm = 1100.0f;
    }

    if (m_currentRpm >= 7400.0f && m_currentGear < m_totalGearCount) {
        m_currentGear++;
        m_currentRpm = 3200.0f;
        Platform::Log("[GEARBOX SHIFT]: Transmission shifted UP to gear " + std::to_string(m_currentGear));
    } else if (m_currentRpm <= 1900.0f && m_currentGear > 1) {
        m_currentGear--;
        m_currentRpm = 5400.0f;
        Platform::Log("[GEARBOX SHIFT]: Transmission shifted DOWN to gear " + std::to_string(m_currentGear));
    }
}

// ============================================================================
// SECTION 3: COMPONENT DAMAGE AND STRUCTURAL INTEGRITY
// ============================================================================

void VehiclePhysics::TakeDamageToWheel(size_t wheelIndex, float damageAmount) {
    if (wheelIndex >= m_wheels.size()) return;
    VehicleModification& wheel = m_wheels[wheelIndex];
    if (wheel.isDetached) return;

    float netDamage = damageAmount - wheel.armorValue;
    if (netDamage < 3.0f) netDamage = 3.0f;
    wheel.health -= netDamage;

    Platform::Log("[VEHICLE DAMAGE]: Suspension node ID " + std::to_string(wheelIndex) + " took hit. Remaining HP: " + std::to_string(wheel.health));

    if (wheel.health < 55.0f && m_vehicleType == VehicleType::WheeledCar) {
        wheel.tirePressurePsi = 0.4f;
        wheel.terrainPassability = 0.25f;
        Platform::Log("[TIRE PUNCTURE]: Tire shredded on node " + std::to_string(wheelIndex) + ". Pressure dropped to 0.4 PSI.");
    }

    if (wheel.health <= 0.0f) {
        wheel.health = 0.0f;
        wheel.tirePressurePsi = 0.0f;
        wheel.terrainPassability = 0.0f;
        wheel.speedMultiplier = 0.0f;
        wheel.isDetached = true;
        Platform::Log("[CRITICAL FAILURE]: Suspension node " + std::to_string(wheelIndex) + " completely sheared off!");
    }
}

void VehiclePhysics::TakeDamageToTitanLeg(size_t legIndex, float damageAmount) {
    if (legIndex >= m_legs.size()) return;
    TitanLegStructure& leg = m_legs[legIndex];
    if (leg.isCrippled) return;

    float netDamage = damageAmount - leg.armorValue;
    if (netDamage < 8.0f) netDamage = 8.0f;
    leg.health -= netDamage;

    Platform::Log("[TITAN ARMOR FAILURE]: Titan actuator leg #" + std::to_string(legIndex) + " impacted. HP: " + std::to_string(leg.health));

    if (leg.health <= 0.0f) {
        leg.health = 0.0f;
        leg.isCrippled = true;
        Platform::Log("[TITAN CRIPPLED]: Hydraulic leg #" + std::to_string(legIndex) + " destroyed. Permanent limp induced.");
    }
}

// ============================================================================
// SECTION 4: MAIN PHYSICS LOOP & TIRE SLIP FRICTION CALCULATIONS
// ============================================================================

void VehiclePhysics::SimulatePhysics(float deltaTime, const Vector3D& moveInput, Vector3D& outVelocity) {
    UpdateTransmission(deltaTime);

    if (m_vehicleType == VehicleType::AircraftDrone) {
        SimulateAerodynamics(deltaTime, moveInput, outVelocity);
        return;
    }

    if (m_vehicleType == VehicleType::TitanHumanoid || m_vehicleType == VehicleType::TitanMultiLegged) {
        SimulateTitanLocomotion(deltaTime, moveInput, outVelocity);
        return;
    }

    if (m_wheels.empty()) {
        outVelocity = Vector3D(0.0f, 0.0f, 0.0f);
        m_cameraShakeIntensity = 0.0f;
        return;
    }

    float totalSuspensionForce = 0.0f;
    for (size_t i = 0; i < m_wheels.size(); ++i) {
        if (!m_wheels[i].isDetached) {
            float springCompression = 0.15f;
            float springF = springCompression * m_suspensionStiffness;
            float dampF = m_velocity.y * m_suspensionDamping;
            totalSuspensionForce += (springF - dampF);
        }
    }

    float totalPass = 0.0f;
    float totalSpeedMod = 0.0f;
    float activeCount = 0.0f;
    float destroyedCount = 0.0f;

    for (const auto& wheel : m_wheels) {
        if (wheel.isDetached) {
            destroyedCount += 1.0f;
            continue;
        }
        totalPass += wheel.terrainPassability;
        totalSpeedMod += wheel.speedMultiplier;
        activeCount += 1.0f;
    }

    if (activeCount == 0.0f) {
        m_velocity = Vector3D(0.0f, 0.0f, 0.0f);
        outVelocity = m_velocity;
        m_cameraShakeIntensity = 1.0f;
        return;
    }

    float avgPass = totalPass / activeCount;
    float avgSpeedM = totalSpeedMod / activeCount;

    float dynamicGrip = m_tireGripCoefficient * avgPass;
    if (m_handbrakeActive) dynamicGrip *= 0.2f;

    float gearFactor = 0.45f + (static_cast<float>(m_currentGear) * 0.19f);
    float finalTopSpeed = m_baseSpeed * avgSpeedM * avgPass * gearFactor * dynamicGrip;

    if (destroyedCount > 0.0f) {
        float structuralPenalty = 1.0f - (destroyedCount / static_cast<float>(m_wheels.size()));
        finalTopSpeed *= structuralPenalty;

        static float shakeClock = 0.0f;
        shakeClock += deltaTime * 20.0f;
        m_cameraShakeIntensity = std::sin(shakeClock) * (destroyedCount * 0.5f);
    } else {
        m_cameraShakeIntensity = 0.0f;
    }

    if (m_handbrakeActive || m_currentBraking > 0.1f) {
        finalTopSpeed *= 0.02f;
    }

    Vector3D targetVelocityVector = moveInput.Normalized() * (finalTopSpeed * std::abs(m_currentThrottle));
    float weightScalar = 1200.0f / m_vehicleMass;
    float smoothingRate = 5.0f * weightScalar * deltaTime;

    m_velocity.x = m_velocity.x + (targetVelocityVector.x - m_velocity.x) * smoothingRate;
    m_velocity.z = m_velocity.z + (targetVelocityVector.z - m_velocity.z) * smoothingRate;
    m_velocity.y = (totalSuspensionForce / m_vehicleMass) * deltaTime;

    outVelocity = m_velocity;
}

// ============================================================================
// SECTION 5: AERODYNAMICS & TITAN LOCOMOTION SUB-ROUTINES
// ============================================================================

void VehiclePhysics::SimulateAerodynamics(float deltaTime, const Vector3D& moveInput, Vector3D& outVelocity) {
    float throttleScalar = std::clamp(m_currentThrottle, 0.0f, 1.0f);
    float targetVelocityMagnitude = m_baseSpeed * throttleScalar * 3.2f;

    if (moveInput.y != 0.0f) {
        m_flightAltitude += moveInput.y * 15.0f * deltaTime;
        if (m_flightAltitude < 4.0f) m_flightAltitude = 4.0f;
        if (m_flightAltitude > 500.0f) m_flightAltitude = 500.0f;
    }

    float aerodynamicDrag = 0.025f * (targetVelocityMagnitude * targetVelocityMagnitude);
    float effectiveThrust = targetVelocityMagnitude - aerodynamicDrag;

    Vector3D flightHeading = moveInput.Normalized();
    m_velocity = flightHeading * std::max(0.0f, effectiveThrust);
    m_velocity.y = (m_flightAltitude - m_velocity.y) * 4.0f * deltaTime;

    outVelocity = m_velocity;
    m_cameraShakeIntensity = throttleScalar * 0.22f;
}

void VehiclePhysics::SimulateTitanLocomotion(float deltaTime, const Vector3D& moveInput, Vector3D& outVelocity) {
    float legDamageFactor = 1.0f;
    for (const auto& leg : m_legs) {
        if (leg.isCrippled) {
            legDamageFactor *= 0.4f;
        }
    }

    float targetMechSpeed = m_baseSpeed * legDamageFactor;
    if (moveInput.Length() > 0.01f) {
        Vector3D travelDir = moveInput.Normalized();
        m_velocity.x = m_velocity.x + (travelDir.x * targetMechSpeed - m_velocity.x) * (7.0f * deltaTime);
        m_velocity.z = m_velocity.z + (travelDir.z * targetMechSpeed - m_velocity.z) * (7.0f * deltaTime);
    } else {
        m_velocity.x = m_velocity.x * (1.0f - (12.0f * deltaTime));
        m_velocity.z = m_velocity.z * (1.0f - (12.0f * deltaTime));
    }

    outVelocity = m_velocity;

    if (moveInput.Length() > 0.1f) {
        static float titanPaceTimer = 0.0f;
        titanPaceTimer += deltaTime * 11.0f;
        m_cameraShakeIntensity = std::abs(std::sin(titanPaceTimer)) * 0.35f;
    } else {
        m_cameraShakeIntensity = 0.0f;
    }
}

// ============================================================================
// SECTION 6: TELEMETRY, WEIGHT BALANCING, AND STATUS QUERIES
// ============================================================================

float VehiclePhysics::GetTotalVehicleWeight() const noexcept {
    float accumWeight = m_vehicleMass;
    for (const auto& wheel : m_wheels) {
        if (!wheel.isDetached) accumWeight += 105.0f;
    }
    for (const auto& leg : m_legs) {
        if (!leg.isCrippled) accumWeight += 1500.0f;
    }
    return accumWeight;
}

bool VehiclePhysics::IsFullyOperational() const noexcept {
    for (const auto& wheel : m_wheels) {
        if (wheel.isDetached || wheel.health < 30.0f) return false;
    }
    for (const auto& leg : m_legs) {
        if (leg.isCrippled || leg.health < 60.0f) return false;
    }
    return true;
}

} // namespace Centralia