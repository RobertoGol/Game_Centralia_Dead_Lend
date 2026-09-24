#pragma once
#include "core/Math3D.hpp"
#include "gameplay/ModificationSystem.hpp"
#include <vector>

namespace Centralia {

class VehiclePhysics : public ModdableEntity {
private:
    std::vector<VehicleModification> m_wheels;
    float m_chassisHealth = 500.0f;
    bool m_isHeavyTracked = false;

    // Внутренние параметры для генерации тряски камеры процессором
    float m_cameraShakeIntensity = 0.0f;

public:
    VehiclePhysics(bool isTracked = false);
    ~VehiclePhysics() = default;

    // Инициализация 4 колес или 2 гусеничных траков тяжелой техники со скриншотов
    void SetupChassis(const VehicleModification& baseModTemplate);

    // Симуляция попадания пули/снаряда в конкретное колесо (из механики расчлененки LDoE)
    void TakeDamageToWheel(size_t wheelIndex, float damageAmount);

    // Ежекадровый обсчет физики давления, сцепления с почвой и крена на CPU
    void SimulatePhysics(float deltaTime, const Vector3D& moveInput, Vector3D& outVelocity);

    // Геттеры для связи с системами шейдеров и 3D-камеры
    float GetCameraShake() const { return m_cameraShakeIntensity; }
    const std::vector<VehicleModification>& GetWheels() const { return m_wheels; }
};

} // namespace Centralia
