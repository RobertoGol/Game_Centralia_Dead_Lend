#include "gameplay/MonsterAISystem.hpp"
#include "platform/Platform.hpp"
#include <cmath>
#include <algorithm>

namespace Centralia {

static float GetDistance3D(const Vector3D& a, const Vector3D& b) {
    return std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2) + std::pow(a.z - b.z, 2));
}

void MonsterAISystem::SpawnTacticalAgent(uint32_t id, const std::string& loreName, AIArchetype archetype, const Vector3D& spawnPos) {
    AIIntelligenceProfile agent{};
    agent.monsterId = id;
    agent.monsterLoreName = loreName;
    agent.archetype = archetype;
    agent.currentPosition = spawnPos;
    agent.patrolTarget = spawnPos;
    agent.basePatrolOrigin = spawnPos;
    
    if (archetype == AIArchetype::SwarmDrone) {
        agent.flankAngle = static_cast<float>((id % 4) * 90) * (3.14159f / 180.0f);
    }
    
    m_simulatedAgents.push_back(agent);
    std::string typeStr = (archetype == AIArchetype::SwarmDrone) ? "РОЙ" : "ОДИНОЧКА";
    Platform::Log("MonsterAISystem: Запущен [" + typeStr + "] '" + loreName + "' ID " + std::to_string(id));
}

void MonsterAISystem::ThrowWeaponCasingDistraction(const Vector3D& casingLandingPos) {
    Platform::Log("[STEALTH MECHANICS]: Брошена гильза. Звук падения в координатах X:" 
                  + std::to_string(casingLandingPos.x) + " Z:" + std::to_string(casingLandingPos.z));

    for (auto& agent : m_simulatedAgents) {
        float distanceToCasing = GetDistance3D(agent.currentPosition, casingLandingPos);

        // Одиночные живые существа (Гули, Бегемоты) переходят к расследованию
        if (agent.archetype == AIArchetype::Individual) {
            if (distanceToCasing <= 20.0f && agent.currentBehavior != BehaviorState::CombatChasing) {
                agent.currentBehavior = BehaviorState::Investigating;
                agent.lastKnownNoiseSource = casingLandingPos;
                Platform::Log("[AI " + agent.monsterLoreName + "]: Услышал падение гильзы. Иду проверять.");
            }
        } 
        // Рой роботов с эшелонированием
        else if (agent.archetype == AIArchetype::SwarmDrone) {
            if (agent.currentBehavior == BehaviorState::CombatChasing) continue;

            // БЛИЖАЙШИЙ ЭШЕЛОН РОЯ: Мгновенная атака точки звука гильзы
            if (distanceToCasing <= SWARM_IMMEDIATE_ATTACK_RADIUS) {
                agent.currentBehavior = BehaviorState::CombatChasing;
                agent.lastKnownNoiseSource = casingLandingPos;
                Platform::Log("[SWARM HIVE MIND]: Ближний дрон ID " + std::to_string(agent.monsterId) + " атакует точку падения гильзы!");
            }
            // СРЕДНИЙ И ДАЛЬНИЙ ЭШЕЛОН: Дебаффнутое стягивание (смещение центра патруля всего на 30%)
            else if (distanceToCasing <= SWARM_MID_DIST_SECTOR_RADIUS) {
                agent.lastKnownNoiseSource = casingLandingPos;
                
                // Боты не идут прямо к гильзе, а слегка смещают зону патрулирования (дебафф интеллекта)
                agent.patrolTarget.x = agent.basePatrolOrigin.x + (casingLandingPos.x - agent.basePatrolOrigin.x) * 0.3f;
                agent.patrolTarget.z = agent.basePatrolOrigin.z + (casingLandingPos.z - agent.basePatrolOrigin.z) * 0.3f;
                agent.patrolTarget.y = casingLandingPos.y;

                Platform::Log("[SWARM HIVE MIND]: Дальний дрон ID " + std::to_string(agent.monsterId) + " скорректировал сектор обхода поближе к шуму.");
            }
        }
    }
}

void MonsterAISystem::ApplyTargetedDamageToMonster(uint32_t monsterId, float damage, bool hitLegs) {
    for (auto& monster : m_simulatedAgents) {
        if (monster.monsterId == monsterId) {
            monster.health -= damage;
            if (hitLegs) {
                monster.areLegsCrippled = 1; // Регистрация повреждения ходовой части
            }
            
            if (monster.health <= 0.0f) {
                monster.health = 0.0f;
                Platform::Log("[AI DEATH]: Агент '" + monster.monsterLoreName + "' ID " + std::to_string(monsterId) + " полностью уничтожен.");
            }
            break;
        }
    }
}

void MonsterAISystem::BroadcastSwarmTarget(const Vector3D& targetPos, uint32_t broadcastingDroneId) {
    Vector3D senderPos{0,0,0};
    for (const auto& agent : m_simulatedAgents) {
        if (agent.monsterId == broadcastingDroneId) {
            senderPos = agent.currentPosition;
            break;
        }
    }

    for (auto& agent : m_simulatedAgents) {
        if (agent.monsterId == broadcastingDroneId || agent.archetype != AIArchetype::SwarmDrone) continue;
        
        float distToSender = GetDistance3D(agent.currentPosition, senderPos);
        if (distToSender <= SWARM_LINK_RADIUS && agent.currentBehavior != BehaviorState::CombatChasing) {
            // Вторичный режим: только 25% дронов улья уходят на фланкирование, остальные штурмуют в лоб
            if (agent.monsterId % 4 == 0) {
                agent.currentBehavior = BehaviorState::FlankingTarget;
            } else {
                agent.currentBehavior = BehaviorState::CombatChasing;
            }
            agent.lastKnownNoiseSource = targetPos;
        }
    }
}

void MonsterAISystem::ProcessAIScriptsTick(float deltaTime, const Player& player, bool isPlayerSprinting, bool isPlayerInGhostSneak) {
    const Vector3D& playerPos = player.GetPosition();
    float playerArmorWeight = player.GetEquippedArmorWeight();

    // Расчет акустического загрязнения: присед пассивно снижает шум за счет дебаффа генерации звука сервоприводами
    float dynamicPlayerNoise = RAW_BASE_STEP_VOLUME + (playerArmorWeight * 0.12f);
    if (isPlayerSprinting) dynamicPlayerNoise += SPRINT_HEAVY_NOISE;
    if (isPlayerInGhostSneak) dynamicPlayerNoise *= GHOST_SNEAK_DEBUFF; // Пассивное глушение звука

    for (auto& agent : m_simulatedAgents) {
        if (agent.health <= 0.0f) continue; // Пропускаем мертвых

        float distanceToPlayer = GetDistance3D(agent.currentPosition, playerPos);

        switch (agent.currentBehavior) {
            
            case BehaviorState::Patrolling: {
                if (distanceToPlayer <= dynamicPlayerNoise) {
                    agent.suspicionLevel += 50.0f * deltaTime;
                    if (agent.suspicionLevel >= 25.0f) {
                        agent.currentBehavior = BehaviorState::Investigating;
                        agent.lastKnownNoiseSource = playerPos;
                        if (agent.archetype == AIArchetype::SwarmDrone) {
                            BroadcastSwarmTarget(playerPos, agent.monsterId);
                        }
                    }
                } else {
                    // Обычный обход. Если это дальний дрон роя, стянутый гильзой, он идет медленнее (дебафф скорости)
                    float distToPatrol = GetDistance3D(agent.currentPosition, agent.patrolTarget);
                    if (distToPatrol > 0.5f) {
                        float patrolSpeed = (agent.patrolTarget.x != agent.basePatrolOrigin.x) ? 1.0f : 1.5f; // Штраф к скорости при сужении кольца
                        agent.currentPosition.x += ((agent.patrolTarget.x - agent.currentPosition.x) / distToPatrol) * patrolSpeed * deltaTime;
                        agent.currentPosition.z += ((agent.patrolTarget.z - agent.currentPosition.z) / distToPatrol) * patrolSpeed * deltaTime;
                    }
                }
                break;
            }

            case BehaviorState::Investigating: {
                float distToNoise = GetDistance3D(agent.currentPosition, agent.lastKnownNoiseSource);
                if (distToNoise > 1.2f) {
                    float dirX = (agent.lastKnownNoiseSource.x - agent.currentPosition.x) / distToNoise;
                    float dirZ = (agent.lastKnownNoiseSource.z - agent.currentPosition.z) / distToNoise;
                    agent.currentPosition.x += dirX * 2.0f * deltaTime;
                    agent.currentPosition.z += dirZ * 2.0f * deltaTime;
                } else {
                    agent.currentBehavior = BehaviorState::SearchingArea;
                    agent.searchRadiusTimer = 4.0f;
                }
                break;
            }

            case BehaviorState::SearchingArea: {
                agent.searchRadiusTimer -= deltaTime;
                if (agent.searchRadiusTimer <= 0.0f) {
                    agent.currentBehavior = BehaviorState::Patrolling;
                    agent.suspicionLevel = 0.0f;
                } else {
                    if (distanceToPlayer < dynamicPlayerNoise && !isPlayerInGhostSneak) {
                        agent.currentBehavior = BehaviorState::CombatChasing;
                    }
                }
                break;
            }

            case BehaviorState::CombatChasing: {
                // Скорость бега режется, если прострелены ноги и не включена ярость
                float chaseSpeed = agent.areLegsCrippled ? (agent.isEnraged ? 6.0f : 2.2f) : 5.5f;

                if (distanceToPlayer > 35.0f) {
                    agent.currentBehavior = BehaviorState::SearchingArea;
                    agent.searchRadiusTimer = 5.0f;
                } else {
                    float dirX = (playerPos.x - agent.currentPosition.x) / distanceToPlayer;
                    float dirZ = (playerPos.z - agent.currentPosition.z) / distanceToPlayer;
                    agent.currentPosition.x += dirX * chaseSpeed * deltaTime;
                    agent.currentPosition.z += dirZ * chaseSpeed * deltaTime;
                }

                // Логика перехода Бегемота в Enrage State
                if (agent.areLegsCrippled && !agent.isEnraged) {
                    agent.isEnraged = 1;
                    agent.baseHearingRadius *= 1.5f;
                    Platform::Log("[AI MECHANICS]: " + agent.monsterLoreName + " перешел в ENRAGE STATE!");
                }
                break;
            }

            case BehaviorState::FlankingTarget: {
            case BehaviorState::FlankingTarget: {
                // Вторичный тактический режим обхода
                float flankRadius = 10.0f;
                float targetX = playerPos.x + std::cos(agent.flankAngle) * flankRadius;
                float targetZ = playerPos.z + std::sin(agent.flankAngle) * flankRadius;
                
                float distToFlank = std::sqrt(std::pow(targetX - agent.currentPosition.x, 2) + std::pow(targetZ - agent.currentPosition.z, 2));
                if (distToFlank > 1.0f) {
                    agent.currentPosition.x += ((targetX - agent.currentPosition.x) / distToFlank) * 4.0f * deltaTime;
                    agent.currentPosition.z += ((targetZ - agent.currentPosition.z) / distToFlank) * 4.0f * deltaTime;
                } else {
                    agent.currentBehavior = BehaviorState::CombatChasing; // Вышел во фланг — переходит к атаке
                }
                break;
            }
        }
    }
}

} // namespace Centralia
