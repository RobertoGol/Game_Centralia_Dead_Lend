#pragma once
#include "core/Math3D.hpp"
#include "platform/Platform.hpp"
#include <vector>
#include <cstdint>

namespace Centralia {

struct MonsterUnit {
    uint32_t monsterId;
    Vector3D position;
    float currentHealth;
    float aggroRange;
    bool isAggressive;
};

class MonsterAISystem {
private:
    std::vector<MonsterUnit> m_activeMonsters;

public:
    MonsterAISystem() = default;
    ~MonsterAISystem() = default;

    MonsterAISystem(const MonsterAISystem&) = delete;
    MonsterAISystem& operator=(const MonsterAISystem&) = delete;

    void SpawnMonster(uint32_t monsterId, const Vector3D& spawnPos);
    void UpdateAllMonsters(float deltaTime, const Vector3D& playerPosition);

    [[nodiscard]] const std::vector<MonsterUnit>& GetActiveMonsters() const noexcept { return m_activeMonsters; }
};

} // namespace Centralia