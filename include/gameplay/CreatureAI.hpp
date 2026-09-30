#pragma once
#include "core/Math3D.hpp"
#include "platform/Platform.hpp"
#include <string>
#include <cstdint>

namespace Centralia {

enum class CreatureState : uint8_t {
    Idle,
    Patrol,
    Alert,
    Combat_Attack,
    Flee,
    Dead
};

class CreatureAI {
private:
    uint32_t m_creatureId;
    CreatureState m_currentState;
    float m_detectionRadius;
    float m_health;
    Vector3D m_currentPosition;

public:
    CreatureAI(uint32_t id, float detectionRadius, float maxHealth);
    ~CreatureAI() = default;

    CreatureAI(const CreatureAI&) = delete;
    CreatureAI& operator=(const CreatureAI&) = delete;

    void UpdateAI(float deltaTime, const Vector3D& playerPosition);
    void TakeDamage(float amount);

    [[nodiscard]] CreatureState GetCurrentState() const noexcept { return m_currentState; }
    [[nodiscard]] float GetHealth() const noexcept { return m_health; }
    [[nodiscard]] bool IsDead() const noexcept { return m_currentState == CreatureState::Dead; }
};

} // namespace Centralia