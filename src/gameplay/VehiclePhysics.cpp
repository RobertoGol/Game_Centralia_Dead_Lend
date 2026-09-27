#include "gameplay/VehiclePhysics.hpp"
#include "platform/Platform.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace Centralia {

VehiclePhysics::VehiclePhysics(bool isTracked) : m_isHeavyTracked(isTracked) {
    m_baseSpeed = 6.0f;  // Базовая скорость техники Централии
    m_baseArmor = 25.0f; // Начальная броня корпуса танка/меха
}

void VehiclePhysics::SetupChassis(const VehicleModification& baseModTemplate) {
    m_wheels.clear();
    // Для стандартного танка или ползущего Титана инициализируем 4 колеса/модуля ходовой
    size_t wheelCount = m_isHeavyTracked ? 2 : 4;
    
    for (size_t i = 0; i < wheelCount; ++i) {
        VehicleModification wheel = baseModTemplate;
        wheel.id = static_cast<uint32_t>(baseModTemplate.id + i);
        wheel.health = baseModTemplate.health;
        wheel.tirePressurePsi = 2.2f; // Исходное рабочее давление в шинах (2.2 Бар)
        wheel.isDetached = false;
        m_wheels.push_back(wheel);
    }
    Platform::Log("VehiclePhysics: Ходовая часть сконфигурирована (" + std::to_string(wheelCount) + " узлов).");
}

void VehiclePhysics::TakeDamageToWheel(size_t wheelIndex, float damageAmount) {
    if (wheelIndex >= m_wheels.size() || m_wheels[wheelIndex].isDetached) return;

    VehicleModification& wheel = m_wheels[wheelIndex];
    
    // Вычитаем броню детали из входящего урона снаряда
    float netDamage = damageAmount - wheel.armorValue;
    if (netDamage < 1.0f) netDamage = 1.0f; // Минимальный урон по модулю

    wheel.health -= netDamage;
    Platform::Log("[COMBAT]: Нанесен урон по колесу [" + std::to_string(wheelIndex) + "]. Оставшееся ХП: " + std::to_string(wheel.health));

    // Проверка падения давления при повреждении (Первая скрытая механика)
    if (wheel.health < 50.0f && !m_isHeavyTracked) {
        wheel.tirePressurePsi = 0.8f; // Покрышка пробита, давление упало
        wheel.terrainPassability = 0.3f; // Колесо начинает жестко вязнуть в грязи
        Platform::Log("[PHYSICS WARNING]: Покрышка пробита! Давление упало до критических 0.8 PSI.");
    }

    // Механика полного отрыва/отстрела колеса
    if (wheel.health <= 0.0f) {
        wheel.health = 0.0f;
        wheel.tirePressurePsi = 0.0f;
        wheel.terrainPassability = 0.0f;
        wheel.isDetached = true;
        wheel.speedMultiplier = 0.0f;
        Platform::Log("[CRITICAL]: Колесо [" + std::to_string(wheelIndex) + "] полностью ОТОРВАНО или уничтожено огнем!");
    }
}

void VehiclePhysics::SimulatePhysics(float deltaTime, const Vector3D& moveInput, Vector3D& outVelocity) {
    float totalPassability = 0.0f;
    float totalSpeedModifier = 0.0f;
    float destroyedWheelsCount = 0.0f;

    for (const auto& wheel : m_wheels) {
        if (wheel.isDetached) {
            destroyedWheelsCount += 1.0f;
            continue;
        }
        totalPassability += wheel.terrainPassability;
        totalSpeedModifier += wheel.speedMultiplier;
    }

    size_t activeCount = m_wheels.size();
    
    // Если колеса еще не инициализированы, гасим скорость, избегая деления на ноль
    if (activeCount == 0) {
        outVelocity = Vector3D(0.0f, 0.0f, 0.0f);
        m_cameraShakeIntensity = 0.0f;
        return;
    }

    float passabilityAverage = totalPassability / static_cast<float>(activeCount);
    float speedModifierAverage = totalSpeedModifier / static_cast<float>(activeCount);

    // Рассчитываем итоговую скорость с учетом сцепления с грунтом Пустоши
    float finalSpeed = GetModifiedSpeed() * speedModifierAverage * passabilityAverage;
    
    // ИСПРАВЛЕНО: Безопасное деление на activeCount защищено от генерации -NaN
    if (destroyedWheelsCount > 0.0f) {
        finalSpeed *= (1.0f - (destroyedWheelsCount / static_cast<float>(activeCount)));
        
        // Генерация амортизационной тряски камеры на CPU (Вторая скрытая механика)
        // Процессор считает синусоиду вибрации сломанной подвески
        static float timeAccumulator = 0.0f;
        timeAccumulator += deltaTime * 10.0f;
        m_cameraShakeIntensity = std::sin(timeAccumulator) * (destroyedWheelsCount * 0.15f);
    } else {
        m_cameraShakeIntensity = 0.0f;
    }

    // Превращаем вектор ввода WASD/Стика в реальную физическую скорость движения в 3D пространстве
    outVelocity = moveInput.Normalize() * finalSpeed;
}

} // namespace Centralia
