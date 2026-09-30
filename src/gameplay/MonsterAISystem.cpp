#include "gameplay/MonsterAISystem.hpp"
#include "gameplay/CreatureAI.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/MapSystem.hpp"
#include "core/MemoryManager.hpp"
#include "platform/Platform.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <random>
#include <cstring>
#include <memory>
#include <chrono>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS, AI DIRECTOR CONFIG & SPATIAL HASHING
// ============================================================================

namespace DirectorConfig {
    constexpr uint32_t SYSTEM_SAVE_MAGIC = 0x44495243; // "DIRC"
    constexpr uint32_t SYSTEM_SAVE_VERSION = 2;

    constexpr float CELL_SIZE = 20.0f; // Размер ячейки пространственной сетки (метры)
    
    // AI Director (Pacing)
    constexpr float TENSION_MAX = 100.0f;
    constexpr float TENSION_COOLDOWN_RATE = 1.5f; // Падение напряженности вне боя
    constexpr float PEAK_DURATION = 30.0f;        // Длительность волны/атаки
    constexpr float RELAXATION_DURATION = 60.0f;  // Гарантированная передышка
    
    // LOD (Level of Detail) Distances
    constexpr float LOD0_DISTANCE_SQ = 50.0f * 50.0f;   // Полный ИИ (BT, Физика, Raycast)
    constexpr float LOD1_DISTANCE_SQ = 150.0f * 150.0f; // Упрощенный ИИ (Steering, Навигация)
    constexpr float CULL_DISTANCE_SQ = 300.0f * 300.0f; // Выгрузка в спящий режим

    // Boids Constants (Роевой интеллект)
    constexpr float SWARM_SEPARATION_RADIUS = 3.0f;
    constexpr float SWARM_COHESION_RADIUS = 15.0f;
    constexpr float SWARM_ALIGNMENT_RADIUS = 10.0f;
}

enum class DirectorState {
    BuildUp,    // Наращивание напряжения (Случайные редкие спавны)
    Peak,       // Пик (Спавн орды, активный бой)
    Relaxation  // Затишье (Спавн заблокирован, игроку дается отдых)
};

// Функция пространственного хеширования 2D координат в 1D ключ
inline uint64_t GetSpatialHash(int cellX, int cellZ) {
    int64_t a = cellX >= 0 ? 2 * static_cast<int64_t>(cellX) : -2 * static_cast<int64_t>(cellX) - 1;
    int64_t b = cellZ >= 0 ? 2 * static_cast<int64_t>(cellZ) : -2 * static_cast<int64_t>(cellZ) - 1;
    return static_cast<uint64_t>((a >= b ? a * a + a + b : a + b * b) / 2);
}

inline void GetCellCoords(const Vector3D& pos, int& cellX, int& cellZ) {
    cellX = static_cast<int>(std::floor(pos.x / DirectorConfig::CELL_SIZE));
    cellZ = static_cast<int>(std::floor(pos.z / DirectorConfig::CELL_SIZE));
}

// ============================================================================
// SECTION 2: SPATIAL GRID (O(1) PROXIMITY SEARCH FOR SWARMS)
// ============================================================================

struct SpatialCell {
    std::vector<CreatureAI*> occupants;
};

class SpatialGrid {
private:
    std::unordered_map<uint64_t, SpatialCell> m_cells;

public:
    void Clear() {
        m_cells.clear();
    }

    void Insert(CreatureAI* creature) {
        if (!creature) return;
        Vector3D pos = creature->GetPosition();
        int cx, cz;
        GetCellCoords(pos, cx, cz);
        m_cells[GetSpatialHash(cx, cz)].occupants.push_back(creature);
    }

    std::vector<CreatureAI*> QueryRadius(const Vector3D& center, float radius) {
        std::vector<CreatureAI*> result;
        int minCx, minCz, maxCx, maxCz;
        
        GetCellCoords(center - Vector3D(radius, 0, radius), minCx, minCz);
        GetCellCoords(center + Vector3D(radius, 0, radius), maxCx, maxCz);

        float radiusSq = radius * radius;

        for (int x = minCx; x <= maxCx; ++x) {
            for (int z = minCz; z <= maxCz; ++z) {
                auto it = m_cells.find(GetSpatialHash(x, z));
                if (it != m_cells.end()) {
                    for (CreatureAI* creature : it->second.occupants) {
                        if ((creature->GetPosition() - center).LengthSquared() <= radiusSq) {
                            result.push_back(creature);
                        }
                    }
                }
            }
        }
        return result;
    }
};

// ============================================================================
// SECTION 3: SYSTEM STATE & PIMPL ARCHITECTURE
// ============================================================================

struct MonsterAISystemImpl {
    std::vector<std::unique_ptr<CreatureAI>> activeMonsters;
    std::vector<std::unique_ptr<CreatureAI>> suspendedMonsters; // Ушли в гибернацию (LOD Culling)
    
    SpatialGrid spatialGrid;
    std::mt19937 rngEngine;

    // AI Director Pacing Variables
    DirectorState directorState = DirectorState::BuildUp;
    float currentTension = 0.0f;
    float stateTimer = 0.0f;
    
    uint32_t nextEntityId = 20000; // Резерв ID для монстров
    
    Player* playerRef = nullptr;

    MonsterAISystemImpl() {
        std::random_device rd;
        rngEngine.seed(rd());
    }

    uint32_t GenerateId() { return nextEntityId++; }
};

MonsterAISystem* MonsterAISystem::s_instance = nullptr;

MonsterAISystem::MonsterAISystem() : m_pImpl(new MonsterAISystemImpl()) {
    if (s_instance) {
        Platform::Log("[AI SYSTEM FATAL]: Двойная инициализация MonsterAISystem!");
        std::terminate();
    }
    s_instance = this;
    Platform::Log("[AI SYSTEM]: Глобальный AI Director и Swarm Manager запущены.");
}

MonsterAISystem::~MonsterAISystem() {
    m_pImpl->activeMonsters.clear();
    m_pImpl->suspendedMonsters.clear();
    delete m_pImpl;
    s_instance = nullptr;
    Platform::Log("[AI SYSTEM]: Менеджер монстров и AI Director выгружены.");
}

MonsterAISystem& MonsterAISystem::GetInstance() {
    if (!s_instance) std::terminate();
    return *s_instance;
}

void MonsterAISystem::BindPlayer(Player* player) {
    m_pImpl->playerRef = player;
}

// ============================================================================
// SECTION 4: SPAWNING, DESPAWNING & LIFECYCLE MANAGEMENT
// ============================================================================

CreatureAI* MonsterAISystem::SpawnMonster(Faction faction, const Vector3D& position) {
    uint32_t id = m_pImpl->GenerateId();
    auto newMonster = std::make_unique<CreatureAI>(id, faction);
    newMonster->SetPosition(position);
    
    // Привязываем физику к ландшафту
    float terrainHeight = MapSystem::GetInstance().GetHeightAt(position.x, position.z);
    newMonster->SetPosition(Vector3D(position.x, terrainHeight, position.z));

    CreatureAI* ptr = newMonster.get();
    m_pImpl->activeMonsters.push_back(std::move(newMonster));
    
    Platform::Log("[AI DIRECTOR]: Заспавнен новый NPC (ID: " + std::to_string(id) + ") Фракция: " + std::to_string(static_cast<int>(faction)));
    return ptr;
}

void MonsterAISystem::RemoveDeadMonsters() {
    auto it = std::remove_if(m_pImpl->activeMonsters.begin(), m_pImpl->activeMonsters.end(),
        [](const std::unique_ptr<CreatureAI>& m) { return m->IsDead(); });

    if (it != m_pImpl->activeMonsters.end()) {
        size_t count = std::distance(it, m_pImpl->activeMonsters.end());
        m_pImpl->activeMonsters.erase(it, m_pImpl->activeMonsters.end());
        Platform::Log("[AI SWARM]: Очищено трупов монстров: " + std::to_string(count));
        
        // Повышаем напряженность при убийстве монстров (Игрок шумит, тратит ресурсы)
        m_pImpl->currentTension = std::min(m_pImpl->currentTension + (count * 5.0f), DirectorConfig::TENSION_MAX);
    }
}

// ============================================================================
// SECTION 5: AI DIRECTOR (PACING, TENSION & PROCEDURAL ENCOUNTERS)
// ============================================================================

void MonsterAISystem::UpdateDirector(float dt) {
    if (!m_pImpl->playerRef) return;

    m_pImpl->stateTimer += dt;
    Vector3D playerPos = m_pImpl->playerRef->GetPosition();

    switch (m_pImpl->directorState) {
        
        // ФАЗА 1: Наращивание напряжения (Build-Up)
        case DirectorState::BuildUp: {
            // Анализ состояния игрока (Генерация Tension)
            float playerHealthPct = m_pImpl->playerRef->GetCurrentHealth() / m_pImpl->playerRef->GetMaxHealth();
            
            // Если игрок ранен, напряженность растет быстрее (паника)
            float tensionGrowth = (1.0f - playerHealthPct) * 2.0f * dt;
            
            // Если игрок бежит или стреляет - напряжение растет
            if (m_pImpl->playerRef->IsSprinting()) tensionGrowth += 1.0f * dt;
            
            m_pImpl->currentTension += tensionGrowth;

            // Случайные одиночные спавны (Атмосфера)
            std::uniform_real_distribution<float> randSpawn(0.0f, 100.0f);
            if (randSpawn(m_pImpl->rngEngine) < (0.5f * dt)) { // 0.5% шанс в секунду
                SpawnAmbientEncounter(playerPos);
            }

            // Переход в фазу атаки, если напряжение достигло пика
            if (m_pImpl->currentTension >= DirectorConfig::TENSION_MAX) {
                m_pImpl->directorState = DirectorState::Peak;
                m_pImpl->stateTimer = 0.0f;
                TriggerHordeEvent(playerPos);
            }
            break;
        }

        // ФАЗА 2: Пик (Peak - Орда, активный бой)
        case DirectorState::Peak: {
            // Во время пика спавны агрессивны, напряжение зафиксировано на 100
            m_pImpl->currentTension = DirectorConfig::TENSION_MAX;

            if (m_pImpl->stateTimer >= DirectorConfig::PEAK_DURATION) {
                // Волна закончилась, переходим к передышке
                m_pImpl->directorState = DirectorState::Relaxation;
                m_pImpl->stateTimer = 0.0f;
                Platform::Log("[AI DIRECTOR]: Пик завершен. Фаза релаксации (Передышка).");
                
                // AudioSystem::PlayMusic("music/ambient_calm.ogg"); // Смена музыки на спокойную
            }
            break;
        }

        // ФАЗА 3: Затишье (Relaxation - Сбор лута)
        case DirectorState::Relaxation: {
            m_pImpl->currentTension -= DirectorConfig::TENSION_COOLDOWN_RATE * dt;
            if (m_pImpl->currentTension < 0.0f) m_pImpl->currentTension = 0.0f;

            // Спавны полностью заблокированы
            
            if (m_pImpl->stateTimer >= DirectorConfig::RELAXATION_DURATION) {
                m_pImpl->directorState = DirectorState::BuildUp;
                m_pImpl->stateTimer = 0.0f;
                Platform::Log("[AI DIRECTOR]: Передышка окончена. Начало фазы нагнетания обстановки.");
            }
            break;
        }
    }
}

void MonsterAISystem::SpawnAmbientEncounter(const Vector3D& playerPos) {
    // Выбор случайной точки вне поля зрения игрока (Radius 40-60 метров)
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * 3.14159f);
    std::uniform_real_distribution<float> radiusDist(40.0f, 60.0f);
    
    float angle = angleDist(m_pImpl->rngEngine);
    float radius = radiusDist(m_pImpl->rngEngine);
    
    Vector3D spawnPos = playerPos + Vector3D(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);
    
    // Спавним 1-3 мобов
    std::uniform_int_distribution<int> countDist(1, 3);
    int count = countDist(m_pImpl->rngEngine);
    
    for (int i = 0; i < count; ++i) {
        SpawnMonster(Faction::Mutants, spawnPos + Vector3D(i * 2.0f, 0, i * 2.0f));
    }
    Platform::Log("[AI DIRECTOR]: Инициирован фоновый спавн (Ambient Encounter) из " + std::to_string(count) + " мутантов.");
}

void MonsterAISystem::TriggerHordeEvent(const Vector3D& playerPos) {
    Platform::Log("[AI DIRECTOR TRIGGER]: ПИК НАПРЯЖЕННОСТИ! ГЕНЕРАЦИЯ ОРДЫ!");
    // AudioSystem::PlayMusic("music/combat_horde.ogg");

    // Спавним 10-15 врагов полукругом перед игроком
    std::uniform_real_distribution<float> angleDist(-1.5f, 1.5f); // ~90 градусов конус
    std::uniform_real_distribution<float> radiusDist(30.0f, 50.0f);
    
    Vector3D playerForward = m_pImpl->playerRef->GetForward();
    float baseAngle = std::atan2(playerForward.z, playerForward.x);

    for (int i = 0; i < 12; ++i) {
        float angle = baseAngle + angleDist(m_pImpl->rngEngine);
        float radius = radiusDist(m_pImpl->rngEngine);
        
        Vector3D spawnPos = playerPos + Vector3D(std::cos(angle) * radius, 0.0f, std::sin(angle) * radius);
        CreatureAI* mutant = SpawnMonster(Faction::Mutants, spawnPos);
        
        // Передаем существам знания о позиции игрока, чтобы они сразу агрессировали
        mutant->ForceTarget(m_pImpl->playerRef->GetEntityId(), playerPos);
    }
}

// ============================================================================
// SECTION 6: SWARM INTELLIGENCE (BOIDS - SEPARATION, ALIGNMENT, COHESION)
// ============================================================================

Vector3D MonsterAISystem::CalculateSwarmSteering(CreatureAI* currentMonster) {
    Vector3D separation(0, 0, 0);
    Vector3D alignment(0, 0, 0);
    Vector3D cohesion(0, 0, 0);
    int neighborsCount = 0;

    Vector3D myPos = currentMonster->GetPosition();
    Faction myFaction = currentMonster->GetFaction();

    // Быстрый запрос соседей через пространственную сетку
    std::vector<CreatureAI*> neighbors = m_pImpl->spatialGrid.QueryRadius(myPos, DirectorConfig::SWARM_COHESION_RADIUS);

    for (CreatureAI* neighbor : neighbors) {
        if (neighbor == currentMonster) continue;
        if (neighbor->GetFaction() != myFaction) continue; // Стая только из своих

        Vector3D neighborPos = neighbor->GetPosition();
        float distSq = (myPos - neighborPos).LengthSquared();

        if (distSq > 0.001f) {
            // 1. Separation (Избегание давки)
            if (distSq < DirectorConfig::SWARM_SEPARATION_RADIUS * DirectorConfig::SWARM_SEPARATION_RADIUS) {
                Vector3D repel = (myPos - neighborPos).Normalized() / std::sqrt(distSq); // Чем ближе, тем сильнее отталкивание
                separation = separation + repel;
            }

            // 2. Alignment (Движение в одном направлении с толпой)
            if (distSq < DirectorConfig::SWARM_ALIGNMENT_RADIUS * DirectorConfig::SWARM_ALIGNMENT_RADIUS) {
                alignment = alignment + neighbor->GetForward();
            }

            // 3. Cohesion (Стремление в центр стаи)
            cohesion = cohesion + neighborPos;
            neighborsCount++;
        }
    }

    if (neighborsCount > 0) {
        alignment = (alignment / static_cast<float>(neighborsCount)).Normalized();
        
        cohesion = cohesion / static_cast<float>(neighborsCount); // Центр масс стаи
        cohesion = (cohesion - myPos).Normalized(); // Вектор к центру
    }

    // Взвешиваем силы
    Vector3D swarmForce = (separation * 1.5f) + (alignment * 1.0f) + (cohesion * 0.8f);
    return swarmForce;
}

// ============================================================================
// SECTION 7: LOD (LEVEL OF DETAIL) CULLING & BACKGROUND SIMULATION
// ============================================================================

void MonsterAISystem::ProcessLODs(float dt) {
    if (!m_pImpl->playerRef) return;

    Vector3D playerPos = m_pImpl->playerRef->GetPosition();

    // 1. Проверка активных на уход в гибернацию
    for (auto it = m_pImpl->activeMonsters.begin(); it != m_pImpl->activeMonsters.end();) {
        float distSq = ((*it)->GetPosition() - playerPos).LengthSquared();
        
        if (distSq > DirectorConfig::CULL_DISTANCE_SQ) {
            // Выгрузка в suspended (Спящий режим - снимает нагрузку с CPU)
            m_pImpl->suspendedMonsters.push_back(std::move(*it));
            it = m_pImpl->activeMonsters.erase(it);
        } else {
            ++it;
        }
    }

    // 2. Проверка спящих на пробуждение (Игрок подошел близко)
    for (auto it = m_pImpl->suspendedMonsters.begin(); it != m_pImpl->suspendedMonsters.end();) {
        float distSq = ((*it)->GetPosition() - playerPos).LengthSquared();
        
        if (distSq <= DirectorConfig::CULL_DISTANCE_SQ) {
            m_pImpl->activeMonsters.push_back(std::move(*it));
            it = m_pImpl->suspendedMonsters.erase(it);
        } else {
            ++it;
        }
    }
}

// ============================================================================
// SECTION 8: MAIN UPDATE LOOP (GRID REBUILD, SWARM, LOD, DIRECTOR)
// ============================================================================

void MonsterAISystem::UpdateTick(float deltaTime) {
    RemoveDeadMonsters();
    UpdateDirector(deltaTime);
    ProcessLODs(deltaTime);

    // 1. Перестроение пространственной сетки для текущего кадра (O(N))
    m_pImpl->spatialGrid.Clear();
    for (auto& monster : m_pImpl->activeMonsters) {
        m_pImpl->spatialGrid.Insert(monster.get());
    }

    Vector3D playerPos = m_pImpl->playerRef ? m_pImpl->playerRef->GetPosition() : Vector3D();

    // 2. Обновление активных монстров с учетом LOD
    for (auto& monster : m_pImpl->activeMonsters) {
        float distSq = (monster->GetPosition() - playerPos).LengthSquared();

        if (distSq < DirectorConfig::LOD0_DISTANCE_SQ) {
            // Полная симуляция (Опрос деревьев поведений, A*, Physics, Raycasts)
            monster->UpdateTick(deltaTime);

            // Инъекция роевого интеллекта в физику монстра
            Vector3D swarmCorrection = CalculateSwarmSteering(monster.get());
            monster->ApplyExternalForce(swarmCorrection * 5.0f); // Применение Steering Force
        } 
        else if (distSq < DirectorConfig::LOD1_DISTANCE_SQ) {
            // LOD 1: Упрощенная симуляция (Только навигация, без тяжелых Raycast и анимаций)
            monster->UpdateSimplifiedTick(deltaTime);
        }
        else {
            // LOD 2: Background Simulation (Просто стоят или идут по прямой)
            // Никаких тяжелых расчетов, ждем ухода в Suspended
        }
    }

    // 3. Обновление фоновых спящих монстров (Раз в 5 секунд симулируем перемещение стай по карте мира)
    // static float backgroundTimer = 0.0f;
    // backgroundTimer += deltaTime;
    // if (backgroundTimer > 5.0f) {
    //     for (auto& sm : m_pImpl->suspendedMonsters) {
    //         // sm->SimulateBackgroundTravel(5.0f);
    //     }
    //     backgroundTimer = 0.0f;
    // }
}

// ============================================================================
// SECTION 9: BINARY SERIALIZATION (SAVING WORLD STATE & SWARMS)
// ============================================================================

uint32_t MonsterAISystem::CalculateChecksum(const std::vector<uint8_t>& buffer) const noexcept {
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : buffer) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

std::vector<uint8_t> MonsterAISystem::SerializeToBinary() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(1024 * 1024); // 1 MB буфер (монстров может быть тысячи)

    const uint8_t* magicPtr = reinterpret_cast<const uint8_t*>(&DirectorConfig::SYSTEM_SAVE_MAGIC);
    buffer.insert(buffer.end(), magicPtr, magicPtr + sizeof(uint32_t));

    const uint8_t* verPtr = reinterpret_cast<const uint8_t*>(&DirectorConfig::SYSTEM_SAVE_VERSION);
    buffer.insert(buffer.end(), verPtr, verPtr + sizeof(uint32_t));

    // 1. Сохранение состояния Режиссера (Director)
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_pImpl->nextEntityId), reinterpret_cast<const uint8_t*>(&m_pImpl->nextEntityId) + sizeof(uint32_t));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_pImpl->directorState), reinterpret_cast<const uint8_t*>(&m_pImpl->directorState) + sizeof(DirectorState));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_pImpl->currentTension), reinterpret_cast<const uint8_t*>(&m_pImpl->currentTension) + sizeof(float));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_pImpl->stateTimer), reinterpret_cast<const uint8_t*>(&m_pImpl->stateTimer) + sizeof(float));

    // Объединяем активных и спящих монстров для сохранения
    uint32_t totalMonsters = static_cast<uint32_t>(m_pImpl->activeMonsters.size() + m_pImpl->suspendedMonsters.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&totalMonsters), reinterpret_cast<const uint8_t*>(&totalMonsters) + sizeof(uint32_t));

    auto serializeCreature = [&buffer](const std::unique_ptr<CreatureAI>& creature) {
        std::vector<uint8_t> cData = creature->SerializeToBinary();
        uint32_t cSize = static_cast<uint32_t>(cData.size());
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&cSize), reinterpret_cast<const uint8_t*>(&cSize) + sizeof(uint32_t));
        buffer.insert(buffer.end(), cData.begin(), cData.end());
    };

    for (const auto& monster : m_pImpl->activeMonsters) serializeCreature(monster);
    for (const auto& monster : m_pImpl->suspendedMonsters) serializeCreature(monster);

    uint32_t checksum = CalculateChecksum(buffer);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&checksum), reinterpret_cast<const uint8_t*>(&checksum) + sizeof(uint32_t));

    Platform::Log("[AI SYSTEM SERIALIZE]: Состояние директора и позиций всех (" + std::to_string(totalMonsters) + ") монстров сохранено в бинарный дамп.");
    return buffer;
}

bool MonsterAISystem::DeserializeFromBinary(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(uint32_t) * 6) return false;

    size_t payloadSize = buffer.size() - sizeof(uint32_t);
    std::vector<uint8_t> payloadData(buffer.begin(), buffer.begin() + payloadSize);
    uint32_t expectedChecksum = CalculateChecksum(payloadData);

    uint32_t storedChecksum = 0;
    std::memcpy(&storedChecksum, buffer.data() + payloadSize, sizeof(uint32_t));

    if (expectedChecksum != storedChecksum) {
        Platform::Log("[AI SYSTEM DESERIALIZE ERROR]: Искажение файла (CRC32 Mismatch).");
        return false;
    }

    size_t cursor = 0;
    uint32_t magic;
    std::memcpy(&magic, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    if (magic != DirectorConfig::SYSTEM_SAVE_MAGIC) return false;

    uint32_t version;
    std::memcpy(&version, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

    std::memcpy(&m_pImpl->nextEntityId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    std::memcpy(&m_pImpl->directorState, buffer.data() + cursor, sizeof(DirectorState)); cursor += sizeof(DirectorState);
    std::memcpy(&m_pImpl->currentTension, buffer.data() + cursor, sizeof(float)); cursor += sizeof(float);
    std::memcpy(&m_pImpl->stateTimer, buffer.data() + cursor, sizeof(float)); cursor += sizeof(float);

    uint32_t totalMonsters;
    std::memcpy(&totalMonsters, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

    m_pImpl->activeMonsters.clear();
    m_pImpl->suspendedMonsters.clear();

    for (uint32_t i = 0; i < totalMonsters; ++i) {
        uint32_t cSize;
        std::memcpy(&cSize, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        
        std::vector<uint8_t> cData(buffer.begin() + cursor, buffer.begin() + cursor + cSize);
        cursor += cSize;

        // Создаем пустышку и восстанавливаем через её внутренний десериализатор
        auto monster = std::make_unique<CreatureAI>(0, Faction::Mutants); 
        if (monster->DeserializeFromBinary(cData)) {
            // Сразу кидаем всех в Suspended, а ProcessLODs() сам раскидает их при первом кадре
            m_pImpl->suspendedMonsters.push_back(std::move(monster));
        }
    }

    Platform::Log("[AI SYSTEM DESERIALIZE]: Успешное восстановление состояния Director'а и популяции мира.");
    return true;
}

} // namespace Centralia