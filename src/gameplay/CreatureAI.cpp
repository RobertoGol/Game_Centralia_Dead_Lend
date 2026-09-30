#include "gameplay/CreatureAI.hpp"
#include "gameplay/MapSystem.hpp"
#include "gameplay/Player.hpp"
#include "platform/Platform.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <queue>
#include <unordered_set>
#include <chrono>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS, MATH & FACTION MATRICES
// ============================================================================

namespace AIConfig {
    constexpr float VISION_CONE_ANGLE_COS = 0.5f; // Угол зрения ~120 градусов (cos(60) = 0.5)
    constexpr float VISION_MAX_DISTANCE = 50.0f;
    constexpr float HEARING_MAX_DISTANCE = 80.0f;
    constexpr float MEMORY_DECAY_TIME = 30.0f;    // ИИ забывает раздражитель через 30 секунд
    constexpr float PATH_RECALCULATION_RATE = 1.5f; // Пересчет A* каждые 1.5 сек

    constexpr uint32_t AI_SAVE_MAGIC = 0x41494254; // "AIBT"
}

enum class NodeStatus {
    Success,
    Failure,
    Running
};

namespace FactionManager {
    std::unordered_map<Faction, std::unordered_map<Faction, FactionStanding>> g_factionMatrix;

    void InitializeMatrix() {
        // Упрощенная инициализация матрицы отношений
        g_factionMatrix[Faction::Mutants][Faction::Player] = FactionStanding::Hostile;
        g_factionMatrix[Faction::Mutants][Faction::Raiders] = FactionStanding::Hostile;
        g_factionMatrix[Faction::Mutants][Faction::Mutants] = FactionStanding::Allied;

        g_factionMatrix[Faction::Raiders][Faction::Player] = FactionStanding::Hostile;
        g_factionMatrix[Faction::Raiders][Faction::Mutants] = FactionStanding::Hostile;
        g_factionMatrix[Faction::Raiders][Faction::Raiders] = FactionStanding::Allied;

        g_factionMatrix[Faction::Wildlife][Faction::Player] = FactionStanding::Neutral; // Атакуют только при угрозе
        g_factionMatrix[Faction::Wildlife][Faction::Mutants] = FactionStanding::Hostile;
    }

    FactionStanding GetStanding(Faction a, Faction b) {
        if (a == b) return FactionStanding::Allied;
        if (g_factionMatrix.empty()) InitializeMatrix();
        return g_factionMatrix[a][b];
    }
}

// ============================================================================
// SECTION 2: SENSORY SYSTEM & BLACKBOARD (MEMORY)
// ============================================================================

struct Stimulus {
    StimulusType type;
    Vector3D position;
    uint32_t sourceEntityId;
    float intensity;
    float timeSinceSensed;
};

class AIBlackboard {
private:
    std::unordered_map<std::string, float> m_floatData;
    std::unordered_map<std::string, Vector3D> m_vectorData;
    std::unordered_map<std::string, uint32_t> m_entityData;
    std::unordered_map<std::string, bool> m_boolData;

    std::vector<Stimulus> m_sensoryMemory;

public:
    void SetFloat(const std::string& key, float value) { m_floatData[key] = value; }
    float GetFloat(const std::string& key, float fallback = 0.0f) const {
        auto it = m_floatData.find(key); return it != m_floatData.end() ? it->second : fallback;
    }

    void SetVector(const std::string& key, const Vector3D& val) { m_vectorData[key] = val; }
    Vector3D GetVector(const std::string& key, const Vector3D& fallback = Vector3D()) const {
        auto it = m_vectorData.find(key); return it != m_vectorData.end() ? it->second : fallback;
    }

    void SetEntity(const std::string& key, uint32_t id) { m_entityData[key] = id; }
    uint32_t GetEntity(const std::string& key, uint32_t fallback = 0) const {
        auto it = m_entityData.find(key); return it != m_entityData.end() ? it->second : fallback;
    }

    void SetBool(const std::string& key, bool val) { m_boolData[key] = val; }
    bool GetBool(const std::string& key, bool fallback = false) const {
        auto it = m_boolData.find(key); return it != m_boolData.end() ? it->second : fallback;
    }

    void RegisterStimulus(const Stimulus& s) {
        // Если стимул от того же источника уже есть - обновляем
        for (auto& mem : m_sensoryMemory) {
            if (mem.sourceEntityId == s.sourceEntityId && mem.type == s.type) {
                mem.position = s.position;
                mem.intensity = std::max(mem.intensity, s.intensity);
                mem.timeSinceSensed = 0.0f;
                return;
            }
        }
        m_sensoryMemory.push_back(s);
    }

    void UpdateMemoryTick(float dt) {
        for (auto it = m_sensoryMemory.begin(); it != m_sensoryMemory.end();) {
            it->timeSinceSensed += dt;
            if (it->timeSinceSensed > AIConfig::MEMORY_DECAY_TIME) {
                it = m_sensoryMemory.erase(it);
            } else {
                ++it;
            }
        }
    }

    const std::vector<Stimulus>& GetMemory() const { return m_sensoryMemory; }
};

// ============================================================================
// SECTION 3: A* PATHFINDING ALGORITHM (NAVGRID IMPLEMENTATION)
// ============================================================================

struct AStarNode {
    int gridX, gridZ;
    float gCost; // Стоимость от старта
    float hCost; // Эвристика до цели
    float fCost() const { return gCost + hCost; }
    AStarNode* parent;

    bool operator>(const AStarNode& other) const {
        return fCost() > other.fCost();
    }
};

struct CompareNodePtr {
    bool operator()(const AStarNode* a, const AStarNode* b) const {
        return a->fCost() > b->fCost();
    }
};

class PathfindingCore {
public:
    static std::vector<Vector3D> FindPath(const Vector3D& startPos, const Vector3D& targetPos) {
        std::vector<Vector3D> path;

        int startX = static_cast<int>(std::round(startPos.x));
        int startZ = static_cast<int>(std::round(startPos.z));
        int targetX = static_cast<int>(std::round(targetPos.x));
        int targetZ = static_cast<int>(std::round(targetPos.z));

        // Если цель слишком далеко - отменяем (защита от зависаний)
        if (std::abs(targetX - startX) > 100 || std::abs(targetZ - startZ) > 100) {
            return path;
        }

        std::priority_queue<AStarNode*, std::vector<AStarNode*>, CompareNodePtr> openSet;
        std::unordered_set<uint64_t> closedSet;
        std::vector<AStarNode*> allAllocatedNodes; // Для ручного освобождения памяти

        auto GetHash = [](int x, int z) -> uint64_t {
            return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) | static_cast<uint32_t>(z);
        };

        auto GetHeuristic = [](int x1, int z1, int x2, int z2) -> float {
            // Манхэттенское расстояние для сетки
            return static_cast<float>(std::abs(x1 - x2) + std::abs(z1 - z2));
        };

        // В реальном движке мы берем это из NavMesh или VoxelGrid
        auto IsWalkable = [](int x, int z) -> bool {
            // Заглушка. Обращаемся к MapSystem для проверки препятствий
            // float height = MapSystem::GetInstance().GetHeightAt(static_cast<float>(x), static_cast<float>(z));
            // if (height > MAX_CLIMB_HEIGHT) return false;
            return true; 
        };

        AStarNode* startNode = new AStarNode{startX, startZ, 0.0f, GetHeuristic(startX, startZ, targetX, targetZ), nullptr};
        openSet.push(startNode);
        allAllocatedNodes.push_back(startNode);

        const int dirX[] = {0, 1, 0, -1, 1, 1, -1, -1};
        const int dirZ[] = {1, 0, -1, 0, 1, -1, 1, -1};

        AStarNode* currentNode = nullptr;
        bool pathFound = false;

        while (!openSet.empty()) {
            currentNode = openSet.top();
            openSet.pop();

            uint64_t currHash = GetHash(currentNode->gridX, currentNode->gridZ);
            if (closedSet.find(currHash) != closedSet.end()) continue;
            closedSet.insert(currHash);

            // Достигли цели
            if (currentNode->gridX == targetX && currentNode->gridZ == targetZ) {
                pathFound = true;
                break;
            }

            // Ограничение на итерации (защита от провалов FPS)
            if (closedSet.size() > 1000) break;

            for (int i = 0; i < 8; ++i) {
                int nx = currentNode->gridX + dirX[i];
                int nz = currentNode->gridZ + dirZ[i];

                if (!IsWalkable(nx, nz)) continue;
                if (closedSet.find(GetHash(nx, nz)) != closedSet.end()) continue;

                float stepCost = (i < 4) ? 1.0f : 1.414f; // Диагонали дороже
                float newGCost = currentNode->gCost + stepCost;

                AStarNode* neighbor = new AStarNode{nx, nz, newGCost, GetHeuristic(nx, nz, targetX, targetZ), currentNode};
                openSet.push(neighbor);
                allAllocatedNodes.push_back(neighbor);
            }
        }

        if (pathFound && currentNode) {
            AStarNode* trace = currentNode;
            while (trace != nullptr) {
                // Преобразуем обратно в мировые координаты, считывая реальную высоту
                float wX = static_cast<float>(trace->gridX);
                float wZ = static_cast<float>(trace->gridZ);
                // float wY = MapSystem::GetInstance().GetHeightAt(wX, wZ);
                path.push_back(Vector3D(wX, 0.0f, wZ)); 
                trace = trace->parent;
            }
            std::reverse(path.begin(), path.end());
        }

        // Очистка памяти графа поиска
        for (AStarNode* node : allAllocatedNodes) {
            delete node;
        }

        // Сглаживание пути (String Pulling)
        // ... (в AAA движке здесь удаляются лишние точки)

        return path;
    }
};

// ============================================================================
// SECTION 4: STEERING BEHAVIORS (MOVEMENT PHYSICS)
// ============================================================================

namespace Steering {
    Vector3D Seek(const Vector3D& currentPos, const Vector3D& targetPos, const Vector3D& currentVelocity, float maxSpeed) {
        Vector3D desiredVelocity = (targetPos - currentPos).Normalized() * maxSpeed;
        return desiredVelocity - currentVelocity;
    }

    Vector3D Flee(const Vector3D& currentPos, const Vector3D& targetPos, const Vector3D& currentVelocity, float maxSpeed) {
        Vector3D desiredVelocity = (currentPos - targetPos).Normalized() * maxSpeed;
        return desiredVelocity - currentVelocity;
    }

    Vector3D Arrive(const Vector3D& currentPos, const Vector3D& targetPos, const Vector3D& currentVelocity, float maxSpeed, float slowingRadius) {
        Vector3D toTarget = targetPos - currentPos;
        float distance = toTarget.Length();
        if (distance <= 0.01f) return Vector3D(0, 0, 0) - currentVelocity; // Полная остановка

        float rampedSpeed = maxSpeed * (distance / slowingRadius);
        float clippedSpeed = std::min(rampedSpeed, maxSpeed);
        Vector3D desiredVelocity = (toTarget / distance) * clippedSpeed;
        return desiredVelocity - currentVelocity;
    }

    Vector3D Wander(Vector3D& wanderTarget, float wanderRadius, float wanderDistance, float wanderJitter) {
        // Псевдослучайное блуждание
        wanderTarget.x += (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * wanderJitter;
        wanderTarget.z += (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * wanderJitter;
        wanderTarget = wanderTarget.Normalized() * wanderRadius;

        Vector3D localTarget = wanderTarget + Vector3D(0, 0, wanderDistance);
        return localTarget; // Трансформация в мировые координаты производится классом AI
    }
}

// ============================================================================
// SECTION 5: BEHAVIOR TREE (BT) FRAMEWORK
// ============================================================================

class BTNode {
public:
    virtual ~BTNode() = default;
    virtual NodeStatus Tick(AIBlackboard& blackboard) = 0;
};

class BTSelector : public BTNode {
private:
    std::vector<BTNode*> m_children;
public:
    void AddChild(BTNode* child) { m_children.push_back(child); }
    ~BTSelector() { for (auto c : m_children) delete c; }

    NodeStatus Tick(AIBlackboard& blackboard) override {
        for (auto child : m_children) {
            NodeStatus status = child->Tick(blackboard);
            if (status != NodeStatus::Failure) {
                return status; // Успех или Выполнение
            }
        }
        return NodeStatus::Failure;
    }
};

class BTSequence : public BTNode {
private:
    std::vector<BTNode*> m_children;
public:
    void AddChild(BTNode* child) { m_children.push_back(child); }
    ~BTSequence() { for (auto c : m_children) delete c; }

    NodeStatus Tick(AIBlackboard& blackboard) override {
        for (auto child : m_children) {
            NodeStatus status = child->Tick(blackboard);
            if (status != NodeStatus::Success) {
                return status; // Провал или Выполнение (прерываем секвенцию)
            }
        }
        return NodeStatus::Success; // Все узлы выполнились успешно
    }
};

// ============================================================================
// SECTION 6: SPECIFIC BEHAVIOR TASKS
// ============================================================================

// Декоратор: Проверка, есть ли активная цель (враг)
class BTCheckHasTarget : public BTNode {
public:
    NodeStatus Tick(AIBlackboard& blackboard) override {
        return blackboard.GetEntity("TargetEnemy", 0) != 0 ? NodeStatus::Success : NodeStatus::Failure;
    }
};

// Задача: Преследование врага
class BTTaskChase : public BTNode {
public:
    NodeStatus Tick(AIBlackboard& blackboard) override {
        uint32_t targetId = blackboard.GetEntity("TargetEnemy", 0);
        if (targetId == 0) return NodeStatus::Failure;

        // В реальности получаем позицию цели из менеджера сущностей
        Vector3D enemyPos = blackboard.GetVector("LastKnownEnemyPos");
        Vector3D myPos = blackboard.GetVector("MyPosition");

        float distSq = (enemyPos - myPos).LengthSquared();
        
        // Если подошли вплотную - конец преследования, можно атаковать
        if (distSq < 4.0f) {
            return NodeStatus::Success;
        }

        // Обновляем цель для системы Steering
        blackboard.SetVector("SteeringTarget", enemyPos);
        blackboard.SetBool("IsRunning", true);

        return NodeStatus::Running;
    }
};

// Задача: Ближняя атака (Melee)
class BTTaskMeleeAttack : public BTNode {
public:
    NodeStatus Tick(AIBlackboard& blackboard) override {
        float lastAttackTime = blackboard.GetFloat("LastAttackTime");
        float currentTime = blackboard.GetFloat("CurrentTime");

        // Кулдаун атаки 1.5 секунды
        if (currentTime - lastAttackTime < 1.5f) {
            return NodeStatus::Running; // Ждем отката
        }

        Platform::Log("[AI BEHAVIOR]: Существо наносит удар в ближнем бою!");
        blackboard.SetFloat("LastAttackTime", currentTime);
        
        // Отправка события урона
        // DamageSystem::ApplyDamage(blackboard.GetEntity("TargetEnemy"), 25.0f);

        return NodeStatus::Success;
    }
};

// Задача: Патрулирование (Wander)
class BTTaskPatrol : public BTNode {
public:
    NodeStatus Tick(AIBlackboard& blackboard) override {
        Vector3D myPos = blackboard.GetVector("MyPosition");
        Vector3D wanderTarget = blackboard.GetVector("WanderLocalTarget", Vector3D(1, 0, 0));
        
        Vector3D localDir = Steering::Wander(wanderTarget, 5.0f, 10.0f, 2.0f);
        blackboard.SetVector("WanderLocalTarget", wanderTarget);

        // Трансформируем локальное направление в мировую цель
        Vector3D myForward = blackboard.GetVector("MyForward");
        Vector3D myRight = Vector3D(0, 1, 0).Cross(myForward).Normalized();
        Vector3D worldTarget = myPos + (myRight * localDir.x) + (myForward * localDir.z);

        blackboard.SetVector("SteeringTarget", worldTarget);
        blackboard.SetBool("IsRunning", false);

        return NodeStatus::Running;
    }
};

// ============================================================================
// SECTION 7: CORE CREATURE AI CLASS (CONTROLLER)
// ============================================================================

struct CreatureAIImpl {
    uint32_t entityId;
    Faction faction;
    
    Vector3D position;
    Vector3D forward;
    Vector3D velocity;
    
    float maxHealth;
    float currentHealth;
    float maxSpeed;
    float maxForce;

    AIBlackboard blackboard;
    BTNode* behaviorTreeRoot;
    
    // Pathfinding Cache
    std::vector<Vector3D> currentPath;
    float timeSinceLastPathCalc;

    // Вспомогательная математика для зрения
    bool HasLineOfSight(const Vector3D& targetPos) {
        Vector3D toTarget = targetPos - position;
        float distance = toTarget.Length();
        if (distance > AIConfig::VISION_MAX_DISTANCE) return false;

        toTarget = toTarget / distance;
        float dotProduct = forward.Dot(toTarget);
        
        if (dotProduct < AIConfig::VISION_CONE_ANGLE_COS) return false; // Вне конуса видимости

        // Raycast проверка препятствий
        // RaycastHit hit;
        // if (PhysicsWorld::Raycast(position + Vector3D(0, 1, 0), toTarget, distance, hit)) {
        //     return false; // Стена перекрыла обзор
        // }
        return true;
    }
};

CreatureAI::CreatureAI(uint32_t id, Faction faction) : m_pImpl(new CreatureAIImpl()) {
    m_pImpl->entityId = id;
    m_pImpl->faction = faction;
    
    m_pImpl->maxHealth = 100.0f;
    m_pImpl->currentHealth = 100.0f;
    m_pImpl->maxSpeed = 5.0f;
    m_pImpl->maxForce = 15.0f;
    m_pImpl->timeSinceLastPathCalc = 0.0f;
    m_pImpl->forward = Vector3D(0, 0, 1);

    // СБОРКА ДЕРЕВА ПОВЕДЕНИЙ (Behavior Tree Construction)
    BTSelector* root = new BTSelector();

    // Ветка 1: Атака (Если есть цель)
    BTSequence* combatSequence = new BTSequence();
    combatSequence->AddChild(new BTCheckHasTarget());
    
    BTSelector* combatActionSelector = new BTSelector();
    combatActionSelector->AddChild(new BTTaskMeleeAttack()); // Пытаемся ударить
    combatActionSelector->AddChild(new BTTaskChase());       // Если не достали - бежим
    
    combatSequence->AddChild(combatActionSelector);
    root->AddChild(combatSequence);

    // Ветка 2: Патрулирование (Если врагов нет)
    root->AddChild(new BTTaskPatrol());

    m_pImpl->behaviorTreeRoot = root;
    Platform::Log("[AI SYSTEM]: Интеллект сущности ID " + std::to_string(id) + " инициализирован (Дерево поведений собрано).");
}

CreatureAI::~CreatureAI() {
    delete m_pImpl->behaviorTreeRoot;
    delete m_pImpl;
}

void CreatureAI::UpdateTick(float deltaTime) {
    if (m_pImpl->currentHealth <= 0.0f) return; // Мертв

    m_pImpl->blackboard.SetFloat("CurrentTime", Platform::GetCurrentTimeSeconds());
    m_pImpl->blackboard.SetVector("MyPosition", m_pImpl->position);
    m_pImpl->blackboard.SetVector("MyForward", m_pImpl->forward);

    // 1. Обновление сенсоров (Слух и Зрение)
    UpdateSensorySystem(deltaTime);

    // 2. Исполнение Дерева Поведений
    if (m_pImpl->behaviorTreeRoot) {
        m_pImpl->behaviorTreeRoot->Tick(m_pImpl->blackboard);
    }

    // 3. Вычисление физики передвижения (Steering & Pathfinding)
    UpdateMovementPhysics(deltaTime);
}

void CreatureAI::UpdateSensorySystem(float deltaTime) {
    m_pImpl->blackboard.UpdateMemoryTick(deltaTime);

    // Очистка старой цели
    uint32_t currentTarget = m_pImpl->blackboard.GetEntity("TargetEnemy", 0);

    // Сканирование памяти на предмет врагов
    const auto& memory = m_pImpl->blackboard.GetMemory();
    float closestDist = 99999.0f;
    uint32_t bestTarget = 0;
    Vector3D bestTargetPos;

    for (const auto& stim : memory) {
        // В реальном коде получаем фракцию сущности stim.sourceEntityId
        Faction stimFaction = Faction::Player; 
        
        if (FactionManager::GetStanding(m_pImpl->faction, stimFaction) == FactionStanding::Hostile) {
            float dist = (stim.position - m_pImpl->position).LengthSquared();
            if (dist < closestDist) {
                closestDist = dist;
                bestTarget = stim.sourceEntityId;
                bestTargetPos = stim.position;
            }
        }
    }

    // Реакция на визуальный контакт (обновление данных в реальном времени)
    // В игровом цикле здесь опрашивается EntityManager
    Vector3D mockPlayerPos(10, 0, 10); // Заглушка
    if (m_pImpl->HasLineOfSight(mockPlayerPos)) {
        Stimulus sightStim{StimulusType::Visual, mockPlayerPos, 1 /*Player ID*/, 1.0f, 0.0f};
        m_pImpl->blackboard.RegisterStimulus(sightStim);
    }

    if (bestTarget != 0) {
        m_pImpl->blackboard.SetEntity("TargetEnemy", bestTarget);
        m_pImpl->blackboard.SetVector("LastKnownEnemyPos", bestTargetPos);
    } else {
        m_pImpl->blackboard.SetEntity("TargetEnemy", 0); // Потеря цели
    }
}

void CreatureAI::UpdateMovementPhysics(float deltaTime) {
    Vector3D steeringForce(0, 0, 0);
    Vector3D targetPos = m_pImpl->blackboard.GetVector("SteeringTarget", m_pImpl->position);
    bool isRunning = m_pImpl->blackboard.GetBool("IsRunning", false);
    float currentMaxSpeed = isRunning ? m_pImpl->maxSpeed * 1.5f : m_pImpl->maxSpeed;

    // A* Pathfinding Logic
    m_pImpl->timeSinceLastPathCalc += deltaTime;
    if (m_pImpl->timeSinceLastPathCalc > AIConfig::PATH_RECALCULATION_RATE && (targetPos - m_pImpl->position).LengthSquared() > 4.0f) {
        m_pImpl->currentPath = PathfindingCore::FindPath(m_pImpl->position, targetPos);
        m_pImpl->timeSinceLastPathCalc = 0.0f;
    }

    // Если есть путь, используем алгоритм Path Following
    if (!m_pImpl->currentPath.empty()) {
        Vector3D nextWaypoint = m_pImpl->currentPath.front();
        if ((nextWaypoint - m_pImpl->position).LengthSquared() < 1.0f) {
            m_pImpl->currentPath.erase(m_pImpl->currentPath.begin()); // Удаляем пройденную точку
            if (!m_pImpl->currentPath.empty()) nextWaypoint = m_pImpl->currentPath.front();
        }

        // Применяем Steering Arrive/Seek к следующей точке
        if (m_pImpl->currentPath.size() == 1) {
            steeringForce = steeringForce + Steering::Arrive(m_pImpl->position, nextWaypoint, m_pImpl->velocity, currentMaxSpeed, 3.0f);
        } else {
            steeringForce = steeringForce + Steering::Seek(m_pImpl->position, nextWaypoint, m_pImpl->velocity, currentMaxSpeed);
        }
    } else {
        // Прямой Seek, если пути нет
        steeringForce = steeringForce + Steering::Arrive(m_pImpl->position, targetPos, m_pImpl->velocity, currentMaxSpeed, 3.0f);
    }

    // Ограничение силы руления (Тяга мышц/двигателя)
    if (steeringForce.LengthSquared() > m_pImpl->maxForce * m_pImpl->maxForce) {
        steeringForce = steeringForce.Normalized() * m_pImpl->maxForce;
    }

    // Применение физики Эйлера (Euler Integration)
    // Ускорение = Сила / Масса (Масса условно 1.0)
    m_pImpl->velocity = m_pImpl->velocity + (steeringForce * deltaTime);
    
    // Ограничение максимальной скорости
    if (m_pImpl->velocity.LengthSquared() > currentMaxSpeed * currentMaxSpeed) {
        m_pImpl->velocity = m_pImpl->velocity.Normalized() * currentMaxSpeed;
    }

    // Обновление позиции
    m_pImpl->position = m_pImpl->position + (m_pImpl->velocity * deltaTime);

    // Плавное вращение (LookAt)
    if (m_pImpl->velocity.LengthSquared() > 0.01f) {
        Vector3D desiredForward = m_pImpl->velocity.Normalized();
        // Сферическая интерполяция (Slerp) или простая линейная (Lerp) для вектора forward
        m_pImpl->forward = (m_pImpl->forward * 0.9f + desiredForward * 0.1f).Normalized();
    }
}

// ============================================================================
// SECTION 8: COMBAT & DAMAGE RESPONSE
// ============================================================================

void CreatureAI::ApplyDamage(float amount, uint32_t attackerId, const Vector3D& hitPosition) {
    if (m_pImpl->currentHealth <= 0.0f) return;

    m_pImpl->currentHealth -= amount;
    Platform::Log("[AI COMBAT]: Существо ID " + std::to_string(m_pImpl->entityId) + " получило урон! Оставшееся ХП: " + std::to_string(m_pImpl->currentHealth));

    if (m_pImpl->currentHealth <= 0.0f) {
        Die();
        return;
    }

    // Реакция на боль: регистрация источника урона в сенсорной памяти (Даже если не видим стрелка)
    Stimulus painStim{StimulusType::Damage, hitPosition, attackerId, amount, 0.0f};
    m_pImpl->blackboard.RegisterStimulus(painStim);

    // Сброс дерева поведений - заставляет ИИ немедленно среагировать на угрозу
    m_pImpl->blackboard.SetEntity("TargetEnemy", attackerId);
}

void CreatureAI::Die() {
    Platform::Log("[AI DEAD]: Существо ID " + std::to_string(m_pImpl->entityId) + " мертво. Включение Ragdoll физики.");
    // m_pImpl->behaviorTreeRoot = nullptr; // Остановка логики
    // PhysicsWorld::EnableRagdoll(m_pImpl->entityId);
    // LootSystem::DropLoot(m_pImpl->entityId, m_pImpl->position);
}

// ============================================================================
// SECTION 9: BINARY SERIALIZATION (SUSPEND/RESUME FOR CHUNK STREAMING)
// ============================================================================

std::vector<uint8_t> CreatureAI::SerializeToBinary() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(256);

    uint32_t magic = AIConfig::AI_SAVE_MAGIC;
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&magic), reinterpret_cast<const uint8_t*>(&magic) + sizeof(uint32_t));

    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_pImpl->entityId), reinterpret_cast<const uint8_t*>(&m_pImpl->entityId) + sizeof(uint32_t));
    
    uint32_t factionInt = static_cast<uint32_t>(m_pImpl->faction);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&factionInt), reinterpret_cast<const uint8_t*>(&factionInt) + sizeof(uint32_t));

    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_pImpl->position), reinterpret_cast<const uint8_t*>(&m_pImpl->position) + sizeof(Vector3D));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_pImpl->forward), reinterpret_cast<const uint8_t*>(&m_pImpl->forward) + sizeof(Vector3D));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_pImpl->currentHealth), reinterpret_cast<const uint8_t*>(&m_pImpl->currentHealth) + sizeof(float));

    // Упаковка текущей цели (TargetEnemy)
    uint32_t targetId = m_pImpl->blackboard.GetEntity("TargetEnemy", 0);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&targetId), reinterpret_cast<const uint8_t*>(&targetId) + sizeof(uint32_t));

    Platform::Log("[AI SERIALIZE]: Состояние существа ID " + std::to_string(m_pImpl->entityId) + " упаковано для выгрузки чанка.");
    return buffer;
}

bool CreatureAI::DeserializeFromBinary(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(uint32_t) * 3 + sizeof(Vector3D) * 2 + sizeof(float)) return false;

    size_t cursor = 0;
    uint32_t magic;
    std::memcpy(&magic, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    if (magic != AIConfig::AI_SAVE_MAGIC) return false;

    std::memcpy(&m_pImpl->entityId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    
    uint32_t factionInt;
    std::memcpy(&factionInt, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    m_pImpl->faction = static_cast<Faction>(factionInt);

    std::memcpy(&m_pImpl->position, buffer.data() + cursor, sizeof(Vector3D)); cursor += sizeof(Vector3D);
    std::memcpy(&m_pImpl->forward, buffer.data() + cursor, sizeof(Vector3D)); cursor += sizeof(Vector3D);
    std::memcpy(&m_pImpl->currentHealth, buffer.data() + cursor, sizeof(float)); cursor += sizeof(float);

    uint32_t targetId;
    std::memcpy(&targetId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    if (targetId != 0) m_pImpl->blackboard.SetEntity("TargetEnemy", targetId);

    Platform::Log("[AI DESERIALIZE]: Состояние существа успешно восстановлено из дампа чанка.");
    return true;
}

} // namespace Centralia