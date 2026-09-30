#include "gameplay/VehiclePhysics.hpp"
#include "platform/Platform.hpp"
#include <algorithm>
#include <cmath>
#include <string>
#include <iostream>

namespace Centralia {

// ============================================================================
// 1. КОНСТРУКТОРЫ, ДЕСТРУКТОРЫ И ИНИЦИАЛИЗАЦИЯ ШАССИ
// ============================================================================

VehiclePhysics::VehiclePhysics(VehicleType type) 
    : m_vehicleType(type),
      m_baseSpeed(12.0f),
      m_baseArmor(50.0f),
      m_currentThrottle(0.0f),
      m_currentSteering(0.0f),
      m_currentBraking(0.0f),
      m_handbrakeActive(false),
      m_vehicleMass(2800.0f),
      m_currentRpm(1000.0f),
      m_maxRpm(7500.0f),
      m_currentGear(1),
      m_totalGearCount(5),
      m_angularVelocity(0.0f),
      m_cameraShakeIntensity(0.0f),
      m_flightAltitude(0.0f),
      m_suspensionStiffness(35000.0f),
      m_suspensionDamping(4000.0f),
      m_tireGripCoefficient(1.8f)
{
    m_velocity = Vector3D(0.0f, 0.0f, 0.0f);
    m_acceleration = Vector3D(0.0f, 0.0f, 0.0f);
    m_wheels.clear();
    m_legs.clear();
}

VehiclePhysics::~VehiclePhysics() {
    m_wheels.clear();
    m_legs.clear();
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
                wheel.health = 150.0f;
                wheel.armorValue = 25.0f;
                wheel.tirePressurePsi = 2.4f;
                wheel.terrainPassability = 1.0f;
                wheel.speedMultiplier = 1.0f;
                wheel.isDetached = false;
            }
            m_vehicleMass = 1800.0f;
            m_baseSpeed = 18.0f;
            Platform::Log("[VEHICLE PHYSICS]: Сконфигурировано колесное шасси 4x4 (Car) с симуляцией подвески.");
            break;
        }
        case VehicleType::Motorcycle: {
            m_wheels.resize(2);
            for (size_t i = 0; i < 2; ++i) {
                VehicleModification& wheel = m_wheels[i];
                wheel = baseModTemplate;
                wheel.id = static_cast<uint32_t>(baseModTemplate.id + i);
                wheel.health = 100.0f;
                wheel.armorValue = 15.0f;
                wheel.tirePressurePsi = 2.2f;
                wheel.terrainPassability = 0.85f;
                wheel.speedMultiplier = 1.4f;
                wheel.isDetached = false;
            }
            m_vehicleMass = 450.0f;
            m_baseSpeed = 24.0f;
            Platform::Log("[VEHICLE PHYSICS]: Сконфигурировано двухколесное шасси (Motorcycle).");
            break;
        }
        case VehicleType::TrackedTank: {
            m_wheels.resize(2);
            for (size_t i = 0; i < 2; ++i) {
                VehicleModification& wheel = m_wheels[i];
                wheel = baseModTemplate;
                wheel.id = static_cast<uint32_t>(baseModTemplate.id + i);
                wheel.health = 450.0f;
                wheel.armorValue = 90.0f;
                wheel.tirePressurePsi = 0.0f;
                wheel.terrainPassability = 1.4f;
                wheel.speedMultiplier = 0.65f;
                wheel.isDetached = false;
            }
            m_vehicleMass = 8500.0f;
            m_baseSpeed = 8.0f;
            Platform::Log("[VEHICLE PHYSICS]: Сконфигурировано гусеничное шасси (Tracked Tank).");
            break;
        }
        case VehicleType::TitanHumanoid:
        case VehicleType::TitanMultiLegged: {
            size_t legCount = (m_vehicleType == VehicleType::TitanHumanoid) ? 2 : 4;
            m_legs.resize(legCount);
            for (size_t i = 0; i < legCount; ++i) {
                TitanLegStructure& leg = m_legs[i];
                leg.legId = static_cast<uint32_t>(i);
                leg.health = 600.0f;
                leg.armorValue = 120.0f;
                leg.isCrippled = false;
                leg.isGrounded = true;
                leg.stepProgress = 1.0f;
            }
            m_vehicleMass = 14000.0f;
            m_baseSpeed = 10.0f;
            Platform::Log("[VEHICLE PHYSICS]: Сконфигурировано шагающее шасси Титана с поддержкой обратной кинематики.");
            break;
        }
        case VehicleType::AircraftDrone: {
            m_vehicleMass = 900.0f;
            m_baseSpeed = 45.0f;
            m_flightAltitude = 50.0f;
            Platform::Log("[VEHICLE PHYSICS]: Сконфигурирована аэродинамическая летная модель (Aircraft / Drone).");
            break;
        }
    }
}

// ============================================================================
// 2. СИСТЕМА УПРАВЛЕНИЯ, ТРАНСМИССИИ И ОБОРОТОВ ДВИГАТЕЛЯ (RPM)
// ============================================================================

void VehiclePhysics::ApplyInputs(float throttle, float steering, float braking, bool handbrake) {
    m_currentThrottle = std::clamp(throttle, -1.0f, 1.0f);
    m_currentSteering = std::clamp(steering, -1.0f, 1.0f);
    m_currentBraking = std::clamp(braking, 0.0f, 1.0f);
    m_handbrakeActive = handbrake;
}

void VehiclePhysics::UpdateTransmission(float deltaTime) {
    if (m_vehicleType == VehicleType::AircraftDrone) return;

    float targetRpm = 1000.0f + (std::abs(m_currentThrottle) * 6500.0f);
    
    if (m_currentRpm < targetRpm) {
        m_currentRpm += 4000.0f * deltaTime;
        if (m_currentRpm > m_maxRpm) m_currentRpm = m_maxRpm;
    } else if (m_currentRpm > targetRpm) {
        m_currentRpm -= 3000.0f * deltaTime;
        if (m_currentRpm < 1000.0f) m_currentRpm = 1000.0f;
    }

    if (m_currentRpm >= 7000.0f && m_currentGear < m_totalGearCount) {
        m_currentGear++;
        m_currentRpm = 3000.0f;
        Platform::Log("[GEARBOX]: Автоматическое переключение КПП на передачу вверх -> " + std::to_string(m_currentGear));
    } else if (m_currentRpm <= 1800.0f && m_currentGear > 1) {
        m_currentGear--;
        m_currentRpm = 5200.0f;
        Platform::Log("[GEARBOX]: Автоматическое переключение КПП на передачу вниз -> " + std::to_string(m_currentGear));
    }
}

// ============================================================================
// 3. УПРАВЛЕНИЕ ПОВРЕЖДЕНИЯМИ КОЛЕС, ГУСЕНИЦ И НОГ ТИТАНОВ
// ============================================================================

void VehiclePhysics::TakeDamageToWheel(size_t wheelIndex, float damageAmount) {
    if (wheelIndex >= m_wheels.size()) return;
    VehicleModification& wheel = m_wheels[wheelIndex];
    if (wheel.isDetached) return;

    float netDamage = damageAmount - wheel.armorValue;
    if (netDamage < 2.0f) netDamage = 2.0f;
    wheel.health -= netDamage;

    Platform::Log("[VEHICLE COMBAT]: Узел ходовой ID " + std::to_string(wheelIndex) + " получил урон. Остаток прочности: " + std::to_string(wheel.health));

    if (wheel.health < 50.0f && m_vehicleType == VehicleType::WheeledCar) {
        wheel.tirePressurePsi = 0.5f;
        wheel.terrainPassability = 0.3f;
        Platform::Log("[PHYSICS WARNING]: Покрышка разорвана! Давление упало до критических 0.5 PSI.");
    }

    if (wheel.health <= 0.0f) {
        wheel.health = 0.0f;
        wheel.tirePressurePsi = 0.0f;
        wheel.terrainPassability = 0.0f;
        wheel.speedMultiplier = 0.0f;
        wheel.isDetached = true;
        Platform::Log("[CRITICAL VEHICLE]: Узел ходовой полностью уничтожен и оторван от рамы!");
    }
}

void VehiclePhysics::TakeDamageToTitanLeg(size_t legIndex, float damageAmount) {
    if (legIndex >= m_legs.size()) return;
    TitanLegStructure& leg = m_legs[legIndex];
    if (leg.isCrippled) return;

    float netDamage = damageAmount - leg.armorValue;
    if (netDamage < 5.0f) netDamage = 5.0f;
    leg.health -= netDamage;

    Platform::Log("[TITAN COMBAT]: Опора шагающего Титана #" + std::to_string(legIndex) + " повреждена. ХП: " + std::to_string(leg.health));

    if (leg.health <= 0.0f) {
        leg.health = 0.0f;
        leg.isCrippled = true;
        Platform::Log("[TITAN CRITICAL]: Привод опоры Титана выведен из строя! Мех перешел в режим хромоты.");
    }
}

// ============================================================================
// 4. ГЛАВНЫЙ ФИЗИЧЕСКИЙ ЦИКЛ СИМУЛЯЦИИ И РАСЧЕТ ТРЕНИЯ ШИН (SLIP RATIO)
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

    // Симуляция физики подвески (Упругость по закону Гука + демпфирование)
    float suspensionForceSum = 0.0f;
    for (size_t i = 0; i < m_wheels.size(); ++i) {
        if (!m_wheels[i].isDetached) {
            float compressionDistance = 0.12f; // Моделирование хода сжатия пружины
            float springForce = compressionDistance * m_suspensionStiffness;
            float dampingForce = m_velocity.y * m_suspensionDamping;
            suspensionForceSum += (springForce - dampingForce);
        }
    }

    float totalPassability = 0.0f;
    float totalSpeedMod = 0.0f;
    float activeCount = 0.0f;
    float destroyedCount = 0.0f;

    for (const auto& wheel : m_wheels) {
        if (wheel.isDetached) {
            destroyedCount += 1.0f;
            continue;
        }
        totalPassability += wheel.terrainPassability;
        totalSpeedMod += wheel.speedMultiplier;
        activeCount += 1.0f;
    }

    if (activeCount == 0.0f) {
        m_velocity = Vector3D(0.0f, 0.0f, 0.0f);
        outVelocity = m_velocity;
        m_cameraShakeIntensity = 1.0f;
        return;
    }

    float avgPassability = totalPassability / activeCount;
    float avgSpeedMod = totalSpeedMod / activeCount;

    // Расчет коэффициента продольного сцепления шин с поверхностью (Tire Friction / Slip)
    float surfaceGrip = m_tireGripCoefficient * avgPassability;
    if (m_handbrakeActive) {
        surfaceGrip *= 0.25f; // Резкое падение сцепления при заносе ручника
    }

    float gearRatioFactor = 0.5f + (static_cast<float>(m_currentGear) * 0.18f);
    float finalSpeed = m_baseSpeed * avgSpeedMod * avgPassability * gearRatioFactor * surfaceGrip;

    if (destroyedCount > 0.0f) {
        float penalty = 1.0f - (destroyedCount / static_cast<float>(m_wheels.size()));
        finalSpeed *= penalty;

        static float shakeTimer = 0.0f;
        shakeTimer += deltaTime * 18.0f;
        m_cameraShakeIntensity = std::sin(shakeTimer) * (destroyedCount * 0.45f);
    } else {
        m_cameraShakeIntensity = 0.0f;
    }

    if (m_handbrakeActive || m_currentBraking > 0.1f) {
        finalSpeed *= 0.05f;
    }

    // Интеграция векторов ускорения и инерции массы транспорта
    Vector3D targetVel = moveInput.Normalized() * (finalSpeed * std::abs(m_currentThrottle));
    
    float massFactor = 1000.0f / m_vehicleMass; // Тяжелая техника разгоняется медленнее
    float responseRate = 4.5f * massFactor * deltaTime;

    m_velocity.x = m_velocity.x + (targetVel.x - m_velocity.x) * responseRate;
    m_velocity.z = m_velocity.z + (targetVel.z - m_velocity.z) * responseRate;
    
    // Добавляем вертикальное колебание от работы подвески
    m_velocity.y = (suspensionForceSum / m_vehicleMass) * deltaTime;

    outVelocity = m_velocity;
}

// ============================================================================
// 5. СПЕЦИАЛИЗИРОВАННЫЕ МОДЕЛИ ФИЗИКИ (АЭРОДИНАМИКА И ТИТАНЫ)
// ============================================================================

void VehiclePhysics::SimulateAerodynamics(float deltaTime, const Vector3D& moveInput, Vector3D& outVelocity) {
    float throttleForce = std::clamp(m_currentThrottle, 0.0f, 1.0f);
    float targetFlightSpeed = m_baseSpeed * throttleForce * 3.0f;

    if (moveInput.y != 0.0f) {
        m_flightAltitude += moveInput.y * 12.0f * deltaTime;
        if (m_flightAltitude < 3.0f) m_flightAltitude = 3.0f;
        if (m_flightAltitude > 450.0f) m_flightAltitude = 450.0f;
    }

    // Симуляция аэродинамического сопротивления воздуха (Air Drag)
    float airResistance = 0.02f * (targetFlightSpeed * targetFlightSpeed);
    float netThrust = targetFlightSpeed - airResistance;

    Vector3D flightDir = moveInput.Normalized();
    m_velocity = flightDir * std::max(0.0f, netThrust);
    m_velocity.y = (m_flightAltitude - m_velocity.y) * 3.0f * deltaTime;

    outVelocity = m_velocity;
    m_cameraShakeIntensity = throttleForce * 0.2f;
}

void VehiclePhysics::SimulateTitanLocomotion(float deltaTime, const Vector3D& moveInput, Vector3D& outVelocity) {
    float legPenaltyFactor = 1.0f;
    for (const auto& leg : m_legs) {
        if (leg.isCrippled) {
            legPenaltyFactor *= 0.45f;
        }
    }

    float titanSpeed = m_baseSpeed * legPenaltyFactor;
    if (moveInput.Length() > 0.01f) {
        Vector3D idealDir = moveInput.Normalized();
        m_velocity.x = m_velocity.x + (idealDir.x * titanSpeed - m_velocity.x) * (6.0f * deltaTime);
        m_velocity.z = m_velocity.z + (idealDir.z * titanSpeed - m_velocity.z) * (6.0f * deltaTime);
    } else {
        m_velocity.x = m_velocity.x * (1.0f - (10.0f * deltaTime));
        m_velocity.z = m_velocity.z * (1.0f - (10.0f * deltaTime));
    }

    outVelocity = m_velocity;

    if (moveInput.Length() > 0.1f) {
        static float titanStepTimer = 0.0f;
        titanStepTimer += deltaTime * 9.5f;
        m_cameraShakeIntensity = std::abs(std::sin(titanStepTimer)) * 0.3f;
    } else {
        m_cameraShakeIntensity = 0.0f;
    }
}

// ============================================================================
// 6. ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ И ТЕЛЕМЕТРИЯ
// ============================================================================

float VehiclePhysics::GetTotalVehicleWeight() const noexcept {
    float totalWeight = m_vehicleMass;
    for (const auto& wheel : m_wheels) {
        if (!wheel.isDetached) totalWeight += 95.0f;
    }
    for (const auto& leg : m_legs) {
        if (!leg.isCrippled) totalWeight += 1200.0f;
    }
    return totalWeight;
}

bool VehiclePhysics::IsFullyOperational() const noexcept {
    for (const auto& wheel : m_wheels) {
        if (wheel.isDetached || wheel.health < 25.0f) return false;
    }
    for (const auto& leg : m_legs) {
        if (leg.isCrippled || leg.health < 50.0f) return false;
    }
    return true;
}

} // namespace Centralia