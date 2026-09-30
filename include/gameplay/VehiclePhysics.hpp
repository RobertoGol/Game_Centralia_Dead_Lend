#pragma once
#include "core/Math3D.hpp"
#include "platform/Platform.hpp"
#include <cstdint>
#include <vector>

namespace Centralia {

struct WheelInfo {
    Vector3D localPosition;
    float suspensionLength = 0.6f;
    float springStrength = 25000.0f;
    float damperRate = 4000.0f;
    bool isGrounded = false;
    float currentCompression = 0.0f;
};

class VehiclePhysics {
private:
    Vector3D m_position;
    Vector3D m_velocity;
    Vector3D m_rotation; 
    std::vector<WheelInfo> m_wheels;
    
    float m_mass = 1800.0f;
    float m_enginePower = 350.0f;
    float m_maxSpeed = 120.0f;
    float m_steeringAngle = 0.0f;

public:
    VehiclePhysics();
    ~VehiclePhysics() = default;

    VehiclePhysics(const VehiclePhysics&) = delete;
    VehiclePhysics& operator=(const VehiclePhysics&) = delete;

    void InitializeVehicle(float mass, float power);
    void UpdatePhysics(float deltaTime, float throttleInput, float steeringInput, float brakeInput);

    [[nodiscard]] const Vector3D& GetPosition() const noexcept { return m_position; }
    [[nodiscard]] const Vector3D& GetVelocity() const noexcept { return m_velocity; }
    [[nodiscard]] float GetCurrentSpeedometerKmh() const noexcept;
    
    void SetPosition(const Vector3D& pos) noexcept { m_position = pos; }
};

} // namespace Centralia