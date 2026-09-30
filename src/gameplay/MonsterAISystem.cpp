#include "gameplay/MonsterAISystem.hpp"
#include "gameplay/Player.hpp"
#include "platform/Platform.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>
#include <random>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTRUCTORS, DESTRUCTORS & MULTI-FACTION INITIALIZATION
// ============================================================================

MonsterAISystem::MonsterAISystem() 
    : m_globalAlertLevel(0.0f),
      m_swarmLeaderId(0),
      m_lastKnownPlayerPosition(0.0f, 0.0f, 0.0f),
      m_playerHasBeenSpotted(false),
      m_tacticalFlankingActive(false),
      m_simulationClock(0.0)
{
    m_entities.clear();
    m_activeDistractions.clear();
    Platform::Log("[AI SYSTEM CONSTRUCTOR]: Universal Multi-Faction AI Controller initialized in memory.");
}

MonsterAISystem::~MonsterAISystem() {
    m_entities.clear();
    m_activeDistractions.clear();
    Platform::Log("[AI SYSTEM DESTRUCTOR]: Multi-Faction AI Controller buffers safely unmapped.");
}

void MonsterAISystem::RegisterEntity(const AIEntity& entity) {
    AIEntity enriched = entity;
    enriched.currentBrainTick = 0.0f;
    enriched.morale = 100.0f;
    enriched.hasCover = false;
    
    // Назначаем базовые параметры в зависимости от фракции
    switch (enriched.faction) {
        case AIFaction::SwarmMonster:
            enriched.personalAggression = 0.9f;
            enriched.morale = 999.0f; // Монстры не ведают страха
            break;
        case AIFaction::SemiSwarmRaider:
            enriched.personalAggression = 0.7f;
            enriched.morale = 80.0f; // Бандиты могут дрогнуть под огнем
            break;
        case AIFaction::LoneOutlaw:
            enriched.personalAggression = 0.5f;
            enriched.morale = 50.0f; // Одиночки предпочитают засады и быстрый отход
            break;
        case AIFaction::AllyNPC:
        case AIFaction::NeutralTrader:
            enriched.personalAggression = 0.1f;
            enriched.morale = 100.0f;
            break;
    }

    m_entities.push_back(enriched);
    Platform::Log("[AI REGISTRATION]: Entity ID " + std::to_string(entity.entityId) + 
                  " registered under faction type: " + std::to_string(static_cast<int>(entity.faction)));
}

void MonsterAISystem::UnregisterEntity(uint32_t entityId) {
    auto it = std::remove_if(m_entities.begin(), m_entities.end(), [entityId](const AIEntity& e) {
        return e.entityId == entityId;
    });
    
    if (it != m_entities.end()) {
        m_entities.erase(it, m_entities.end());
        Platform::Log("[AI UNREGISTRATION]: Entity ID " + std::to_string(entityId) + " purged from AI simulation.");
    }
}

// ============================================================================
// SECTION 2: SOPHISTICATED PERCEPTION & FACTION-SPECIFIC SENSORY REACTIONS
// ============================================================================

void MonsterAISystem::RegisterSoundStimulus(const Vector3D& soundOrigin, float loudnessRadius, float intensity) {
    Platform::Log("[AI SENSORY ACOUSTIC]: Sound wave propagated from X: " + std::to_string(soundOrigin.x) + 
                  " Radius: " + std::to_string(loudnessRadius) + " Intensity: " + std::to_string(intensity));

    for (auto& entity : m_entities) {
        if (entity.isDead) continue;

        float distSquared = (entity.position - soundOrigin).LengthSquared();
        float effectiveRadius = loudnessRadius * entity.hearingSensitivity;

        if (distSquared <= (effectiveRadius * effectiveRadius)) {
            // Реакция зависит от фракции
            if (entity.faction == AIFaction::SwarmMonster) {
                if (entity.currentState == AIState::Idle || entity.currentState == AIState::Patrol) {
                    entity.currentState = AIState::InvestigateSound;
                    entity.investigationTarget = soundOrigin;
                    Platform::Log("[SWARM ACOUSTIC REACT]: Monster #" + std::to_string(entity.entityId) + " rushing to sound source.");
                }
            } else if (entity.faction == AIFaction::SemiSwarmRaider) {
                if (entity.currentState == AIState::Idle || entity.currentState == AIState::Patrol) {
                    entity.currentState = AIState::TakeCover; // Бандиты ищут укрытие при шуме
                    entity.investigationTarget = soundOrigin;
                    Platform::Log("[RAIDER ACOUSTIC REACT]: Raider #" + std::to_string(entity.entityId) + " taking defensive cover position.");
                }
            } else if (entity.faction == AIFaction::LoneOutlaw) {
                // Изгои занимают позицию для засады
                entity.currentState = AIState::TakeCover;
                entity.investigationTarget = soundOrigin;
                Platform::Log("[OUTLAW ACOUSTIC REACT]: Outlaw #" + std::to_string(entity.entityId) + " going stealth / ambush mode.");
            }
        }
    }
}

void MonsterAISystem::RegisterDistractionObject(const Vector3D& dropPosition, uint32_t distractionType) {
    DistractionEvent distraction;
    distraction.position = dropPosition;
    distraction.type = distractionType;
    distraction.lifetimeTimer = 25.0f;

    m_activeDistractions.push_back(distraction);
    Platform::Log("[AI DISTRACTION]: Environmental decoy / shell casing dropped at X: " + std::to_string(dropPosition.x));

    for (auto& entity : m_entities) {
        if (entity.isDead || entity.currentState == AIState::Combat) continue;

        float distSq = (entity.position - dropPosition).LengthSquared();
        if (distSq <= 18.0f * 18.0f) {
            if (entity.faction == AIFaction::SwarmMonster || entity.faction == AIFaction::SemiSwarmRaider) {
                entity.currentState = AIState::InvestigateDistraction;
                entity.investigationTarget = dropPosition;
                Platform::Log("[AI DISTRACTION REACT]: Unit #" + std::to_string(entity.entityId) + " diverted to inspect decoy.");
            }
        }
    }
}

// ============================================================================
// SECTION 3: HIERARCHY, RAIDER MORALE & SWARM COORDINATION
// ============================================================================

void MonsterAISystem::UpdateFactionHierarchies() {
    if (m_entities.empty()) return;

    uint32_t topSwarmLeader = 0;
    float maxSwarmPower = -1.0f;

    for (const auto& entity : m_entities) {
        if (entity.isDead) continue;

        if (entity.faction == AIFaction::SwarmMonster) {
            float power = entity.health + (entity.personalAggression * 100.0f);
            if (power > maxSwarmPower) {
                maxSwarmPower = power;
                topSwarmLeader = entity.entityId;
            }
        }
    }
    m_swarmLeaderId = topSwarmLeader;
}

void MonsterAISystem::BroadcastGlobalAlert(const Vector3D& threatPos) {
    m_playerHasBeenSpotted = true;
    m_lastKnownPlayerPosition = threatPos;
    m_globalAlertLevel = 100.0f;
    m_tacticalFlankingActive = true;

    for (auto& entity : m_entities) {
        if (entity.isDead || entity.faction == AIFaction::AllyNPC || entity.faction == AIFaction::NeutralTrader) continue;

        entity.currentState = AIState::Combat;
        entity.lastKnownTargetPos = threatPos;
        entity.stateTimer = 0.0f;
    }
    Platform::Log("[GLOBAL ALERT]: Threat broadcasted across wasteland. Hostile factions entering combat state.");
}

// ============================================================================
// SECTION 4: FACTION-SPECIFIC FINITE STATE MACHINE (FSM)
// ============================================================================

void MonsterAISystem::UpdateAI(float deltaTime, const Player& player) {
    m_simulationClock += static_cast<double>(deltaTime);
    UpdateFactionHierarchies();

    // Очистка старых отвлекающих факторов
    for (auto it = m_activeDistractions.begin(); it != m_activeDistractions.end();) {
        it->lifetimeTimer -= deltaTime;
        if (it->lifetimeTimer <= 0.0f) {
            it = m_activeDistractions.erase(it);
        } else {
            ++it;
        }
    }

    const Vector3D& playerPos = player.GetPosition();

    for (auto& entity : m_entities) {
        if (entity.isDead) continue;

        entity.currentBrainTick += deltaTime;
        if (entity.currentBrainTick < 0.12f) continue; // Оптимизация тиков ИИ
        entity.currentBrainTick = 0.0f;

        // Проверка дистанции видимости
        float distToPlayerSq = (entity.position - playerPos).LengthSquared();
        bool hasVision = (distToPlayerSq <= (entity.visionRange * entity.visionRange));

        if (hasVision && (entity.faction == AIFaction::SwarmMonster || entity.faction == AIFaction::SemiSwarmRaider || entity.faction == AIFaction::LoneOutlaw)) {
            if (entity.currentState != AIState::Combat) {
                Platform::Log("[AI SIGHT]: Entity #" + std::to_string(entity.entityId) + " spotted target!");
                BroadcastGlobalAlert(playerPos);
            }
        }

        // ====================================================================
        // РАЗДЕЛЕНИЕ ЛОГИКИ ПО ФРАКЦИЯМ
        // ====================================================================

        switch (entity.faction) {
            case AIFaction::SwarmMonster: {
                // Логика слепого роя (прежняя сложная итерация преследования и штурма)
                switch (entity.currentState) {
                    case AIState::Idle:
                        entity.stateTimer += 0.12f;
                        if (entity.stateTimer >= 5.0f) { entity.currentState = AIState::Patrol; entity.stateTimer = 0.0f; }
                        break;
                    case AIState::Patrol: {
                        Vector3D pStep = Vector3D(std::sin(m_simulationClock + entity.entityId) * 1.2f, 0.0f, std::cos(m_simulationClock + entity.entityId) * 1.2f);
                        entity.position = entity.position + (pStep * (entity.moveSpeed * 0.3f * 0.12f));
                        break;
                    }
                    case AIState::InvestigateSound:
                    case AIState::InvestigateDistraction: {
                        Vector3D dir = entity.investigationTarget - entity.position;
                        if (dir.LengthSquared() < 2.0f) {
                            entity.stateTimer += 0.12f;
                            if (entity.stateTimer >= 4.0f) { entity.currentState = AIState::Patrol; entity.stateTimer = 0.0f; }
                        } else {
                            entity.position = entity.position + (dir.Normalized() * (entity.moveSpeed * 0.7f * 0.12f));
                        }
                        break;
                    }
                    case AIState::Combat: {
                        entity.lastKnownTargetPos = playerPos;
                        Vector3D toPlayer = playerPos - entity.position;
                        if (toPlayer.LengthSquared() > (entity.visionRange * entity.visionRange * 2.0f)) {
                            entity.currentState = AIState::InvestigateSound;
                            entity.investigationTarget = playerPos;
                        } else if (toPlayer.LengthSquared() > 3.0f) {
                            entity.position = entity.position + (toPlayer.Normalized() * (entity.moveSpeed * 1.15f * 0.12f));
                        }
                        break;
                    }
                    default: break;
                }
                break;
            }

            case AIFaction::SemiSwarmRaider: {
                // Бандиты: используют укрытия, оценивают мораль, отступают при ранении
                if (entity.morale < 25.0f && entity.currentState != AIState::Retreat) {
                    entity.currentState = AIState::Retreat;
                    Platform::Log("[RAIDER MORALE BREAK]: Raider #" + std::to_string(entity.entityId) + " panicking and retreating!");
                }

                switch (entity.currentState) {
                    case AIState::Idle:
                    case AIState::Patrol: {
                        Vector3D rStep = Vector3D(std::cos(m_simulationClock * 0.5f) * 2.0f, 0.0f, std::sin(m_simulationClock * 0.5f) * 2.0f);
                        entity.position = entity.position + (rStep * (entity.moveSpeed * 0.4f * 0.12f));
                        break;
                    }
                    case AIState::TakeCover: {
                        // Поиск укрытия и ожидание
                        entity.stateTimer += 0.12f;
                        if (entity.stateTimer >= 6.0f) {
                            entity.currentState = AIState::Combat;
                            entity.stateTimer = 0.0f;
                        }
                        break;
                    }
                    case AIState::Combat: {
                        Vector3D toPl = playerPos - entity.position;
                        if (toPl.LengthSquared() > 15.0f * 15.0f) {
                            // Бандиты держат среднюю дистанцию для стрельбы из укрытия
                            entity.position = entity.position + (toPl.Normalized() * (entity.moveSpeed * 0.8f * 0.12f));
                        }
                        break;
                    }
                    case AIState::Retreat: {
                        // Бегство от игрока в безопасную зону
                        Vector3D away = entity.position - playerPos;
                        entity.position = entity.position + (away.Normalized() * (entity.moveSpeed * 1.4f * 0.12f));
                        break;
                    }
                    default: break;
                }
                break;
            }

            case AIFaction::LoneOutlaw: {
                // Изгои: одиночные засады, скрытность
                switch (entity.currentState) {
                    case AIState::Idle:
                    case AIState::Patrol: {
                        // Медленное скрытное перемещение
                        break;
                    }
                    case AIState::TakeCover: {
                        // Засада в заброшенном здании
                        break;
                    }
                    case AIState::Combat: {
                        // Одиночка атакует из тени и меняет позицию
                        Vector3D flankDir = playerPos - entity.position;
                        entity.position = entity.position + (flankDir.Normalized() * (entity.moveSpeed * 1.3f * 0.12f));
                        break;
                    }
                    default: break;
                }
                break;
            }

            case AIFaction::AllyNPC:
            case AIFaction::NeutralTrader: {
                // Мирные NPC: стоят на месте, торгуют или следуют за игроком
                break;
            }
        }
    }
}

// ============================================================================
// SECTION 5: CORPSE REGISTRATION AND FACTION SOCIAL REACTIONS
// ============================================================================

void MonsterAISystem::RegisterDeadEntityBody(const Vector3D& bodyPosition, uint32_t deadEntityId) {
    Platform::Log("[AI CORPSE]: Fallen entity registered at X: " + std::to_string(bodyPosition.x));

    AIEntity* nearestAvailable = nullptr;
    float minDstSq = 999999.0f;

    for (auto& entity : m_entities) {
        if (entity.isDead || entity.entityId == deadEntityId) continue;
        if (entity.faction == AIFaction::AllyNPC || entity.faction == AIFaction::NeutralTrader) continue;

        float dSq = (entity.position - bodyPosition).LengthSquared();
        if (dSq < minDstSq) {
            minDstSq = dSq;
            nearestAvailable = &entity;
        }
    }

    if (nearestAvailable && nearestAvailable->currentState != AIState::Combat) {
        nearestAvailable->currentState = AIState::SearchDeadBody;
        nearestAvailable->investigationTarget = bodyPosition;
        nearestAvailable->stateTimer = 0.0f;
        Platform::Log("[FACTION REACTION]: Unit #" + std::to_string(nearestAvailable->entityId) + " dispatched to investigate dead comrade.");
    }
}

// ============================================================================
// SECTION 6: TELEMETRY AND STATUS API
// ============================================================================

float MonsterAISystem::GetGlobalAlertLevel() const noexcept {
    return m_globalAlertLevel;
}

uint32_t MonsterAISystem::GetActiveEntityCount() const noexcept {
    uint32_t count = 0;
    for (const auto& e : m_entities) {
        if (!e.isDead) count++;
    }
    return count;
}

bool MonsterAISystem::IsPlayerSpotted() const noexcept {
    return m_playerHasBeenSpotted;
}

} // namespace Centralia