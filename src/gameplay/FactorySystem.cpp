#include "gameplay/FactorySystem.hpp"
#include "platform/Platform.hpp"
#include "core/MemoryManager.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/MapSystem.hpp"

// В реальном движке здесь будут инклуды для ИИ монстров и урона
// #include "gameplay/CreatureAI.hpp"
// #include "gameplay/DamageSystem.hpp"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <memory>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS, ENUMS & ARCHITECTURE DEFINITIONS
// ============================================================================

namespace FactoryConfig {
    constexpr uint32_t FACTORY_SAVE_MAGIC = 0x46414354; // "FACT"
    constexpr uint32_t FACTORY_SAVE_VERSION = 2;
    constexpr float BELT_SPEED_TIER_1 = 2.0f; // метров в секунду
    constexpr float BELT_SPEED_TIER_2 = 4.5f;
    constexpr float TURRET_SCAN_RADIUS = 35.0f;
    constexpr float GLOBAL_TICK_RATE = 0.05f; // Физика фабрики обновляется 20 раз в сек
}

enum class PowerNodeType {
    Generator,      // Производит энергию (Дизель, Солнечная панель)
    Consumer,       // Потребляет энергию (Турель, Помпа, Освещение)
    Relay,          // Промежуточный столб / кабель
    Battery         // Накопитель энергии (Аккумулятор)
};

enum class BeltItemType {
    Ore, Scrap, Ammo, Medicine, Food, Weapon
};

// ============================================================================
// SECTION 2: GRAPH NODES & FACTORY ENTITIES
// ============================================================================

struct BeltItem {
    uint32_t itemId;
    uint32_t quantity;
    float positionOffset; // От 0.0 до 1.0 на сегменте ленты
};

struct ConveyorNode {
    uint32_t instanceId;
    Vector3D position;
    Vector3D forward;
    float length;
    uint32_t tier; // 1, 2, 3
    
    std::vector<BeltItem> itemsOnBelt;
    
    uint32_t nextNodeId; // ID ленты/контейнера, куда сбрасываем лут
    uint32_t prevNodeId;
};

struct PowerNode {
    uint32_t instanceId;
    PowerNodeType type;
    Vector3D position;
    
    float powerGeneration;   // Для генераторов (Ватт)
    float powerConsumption;  // Для потребителей (Ватт)
    float currentCharge;     // Для батарей
    float maxCapacity;       // Для батарей
    
    bool isPowered;          // Включен ли прибор сейчас
    bool isTurnedOnByUser;   // Ручной тумблер ВКЛ/ВЫКЛ
    
    uint32_t gridNetworkId;  // К какой подсети подключен
    std::vector<uint32_t> connectedWires; // ID связанных узлов (Граф)
};

struct ExtractorNode {
    uint32_t instanceId;
    uint32_t powerNodeId; // Ссылка на электрическую часть
    uint32_t outputBeltId; // Куда выплевывать ресурсы
    
    uint32_t producedItemId;
    uint32_t productionYield;
    float cycleTimeSeconds;
    float currentCycleProgress;
    
    bool requiresWater;
    bool isOperating;
};

struct AutomatedTurret {
    uint32_t instanceId;
    uint32_t powerNodeId;
    
    Vector3D position;
    float currentYaw;
    float currentPitch;
    
    uint32_t targetEntityId;
    float fireRateTimer;
    float fireRateCooldown;
    uint32_t ammoItemId;
    uint32_t ammoLoaded;
    
    bool isOperational;
};

struct PowerGrid {
    uint32_t gridId;
    float totalGeneration;
    float totalConsumption;
    float batteryStorage;
    float batteryMaxCapacity;
    bool isOverloaded; // Блэкаут: потребление > генерации
    std::vector<uint32_t> nodeIds;
};

// ============================================================================
// SECTION 3: SYSTEM IMPLEMENTATION AND INTERNAL MEMORY
// ============================================================================

struct FactorySystemImpl {
    uint32_t nextInstanceId = 1000;
    
    std::unordered_map<uint32_t, PowerNode> powerNodes;
    std::unordered_map<uint32_t, PowerGrid> activeGrids;
    
    std::unordered_map<uint32_t, ConveyorNode> conveyors;
    std::unordered_map<uint32_t, ExtractorNode> extractors;
    std::unordered_map<uint32_t, AutomatedTurret> turrets;
    
    float tickAccumulator = 0.0f;
    uint32_t nextGridId = 1;

    // Вспомогательная функция для генерации уникальных ID
    uint32_t GetNextId() { return nextInstanceId++; }
};

FactorySystem* FactorySystem::s_instance = nullptr;

FactorySystem::FactorySystem() : m_pImpl(new FactorySystemImpl()) {
    if (s_instance) {
        Platform::Log("[FACTORY FATAL]: Двойная инициализация FactorySystem!");
        std::terminate();
    }
    s_instance = this;
    Platform::Log("[FACTORY SYSTEM]: Подсистема автоматизации строительства баз инициализирована.");
}

FactorySystem::~FactorySystem() {
    delete m_pImpl;
    s_instance = nullptr;
    Platform::Log("[FACTORY SYSTEM]: Память подсистемы фабрик освобождена.");
}

FactorySystem& FactorySystem::GetInstance() {
    if (!s_instance) std::terminate();
    return *s_instance;
}

// ============================================================================
// SECTION 4: POWER GRID MANAGEMENT & GRAPH TRAVERSAL (BFS)
// ============================================================================

uint32_t FactorySystem::CreatePowerNode(PowerNodeType type, const Vector3D& position, float genOutput, float consumption) {
    uint32_t id = m_pImpl->GetNextId();
    PowerNode node;
    node.instanceId = id;
    node.type = type;
    node.position = position;
    node.powerGeneration = genOutput;
    node.powerConsumption = consumption;
    node.currentCharge = 0.0f;
    node.maxCapacity = (type == PowerNodeType::Battery) ? 10000.0f : 0.0f;
    node.isPowered = false;
    node.isTurnedOnByUser = true;
    node.gridNetworkId = 0;
    
    m_pImpl->powerNodes[id] = node;
    
    // Пересчет графа при добавлении нового узла
    RecalculatePowerGrids();
    return id;
}

bool FactorySystem::ConnectPowerNodes(uint32_t nodeA, uint32_t nodeB) {
    if (nodeA == nodeB) return false;
    
    auto itA = m_pImpl->powerNodes.find(nodeA);
    auto itB = m_pImpl->powerNodes.find(nodeB);
    
    if (itA == m_pImpl->powerNodes.end() || itB == m_pImpl->powerNodes.end()) return false;
    
    // Ограничение на длину провода (например, 20 метров)
    float distanceSq = (itA->second.position - itB->second.position).LengthSquared();
    if (distanceSq > 400.0f) {
        Platform::Log("[FACTORY ERROR]: Слишком большое расстояние для прокладки электрокабеля.");
        return false;
    }

    // Проверка на дубликат
    auto& wiresA = itA->second.connectedWires;
    if (std::find(wiresA.begin(), wiresA.end(), nodeB) != wiresA.end()) return false;

    // Двунаправленная связь графа
    itA->second.connectedWires.push_back(nodeB);
    itB->second.connectedWires.push_back(nodeA);
    
    Platform::Log("[FACTORY GRID]: Кабель проложен между узлами " + std::to_string(nodeA) + " и " + std::to_string(nodeB));
    
    RecalculatePowerGrids();
    return true;
}

void FactorySystem::RecalculatePowerGrids() {
    m_pImpl->activeGrids.clear();
    m_pImpl->nextGridId = 1;
    
    std::unordered_set<uint32_t> visitedNodes;

    // Алгоритм поиска в ширину (BFS) для выделения изолированных подсетей
    for (auto& [nodeId, node] : m_pImpl->powerNodes) {
        if (visitedNodes.find(nodeId) != visitedNodes.end()) continue;

        // Обнаружена новая изолированная сеть
        PowerGrid newGrid;
        newGrid.gridId = m_pImpl->nextGridId++;
        newGrid.totalGeneration = 0.0f;
        newGrid.totalConsumption = 0.0f;
        newGrid.batteryStorage = 0.0f;
        newGrid.batteryMaxCapacity = 0.0f;
        newGrid.isOverloaded = false;

        std::queue<uint32_t> bfsQueue;
        bfsQueue.push(nodeId);
        visitedNodes.insert(nodeId);

        while (!bfsQueue.empty()) {
            uint32_t currentId = bfsQueue.front();
            bfsQueue.pop();

            PowerNode& current = m_pImpl->powerNodes[currentId];
            current.gridNetworkId = newGrid.gridId;
            newGrid.nodeIds.push_back(currentId);

            // Суммируем генерацию и потребление только для включенных приборов
            if (current.isTurnedOnByUser) {
                if (current.type == PowerNodeType::Generator) {
                    newGrid.totalGeneration += current.powerGeneration;
                } else if (current.type == PowerNodeType::Consumer) {
                    newGrid.totalConsumption += current.powerConsumption;
                } else if (current.type == PowerNodeType::Battery) {
                    newGrid.batteryStorage += current.currentCharge;
                    newGrid.batteryMaxCapacity += current.maxCapacity;
                }
            }

            // Добавляем соседей в очередь
            for (uint32_t neighborId : current.connectedWires) {
                if (visitedNodes.find(neighborId) == visitedNodes.end()) {
                    visitedNodes.insert(neighborId);
                    bfsQueue.push(neighborId);
                }
            }
        }

        // Определение статуса электросети (Перегрузка / Блэкаут)
        if (newGrid.totalConsumption > newGrid.totalGeneration) {
            // Пытаемся покрыть дефицит за счет аккумуляторов (будет реализовано в UpdateTick)
            if (newGrid.totalGeneration + newGrid.batteryStorage < newGrid.totalConsumption) {
                newGrid.isOverloaded = true;
            }
        }

        // Применяем статус питания ко всем узлам в этой подсети
        for (uint32_t nId : newGrid.nodeIds) {
            PowerNode& pNode = m_pImpl->powerNodes[nId];
            if (pNode.type == PowerNodeType::Consumer) {
                pNode.isPowered = pNode.isTurnedOnByUser && !newGrid.isOverloaded;
            } else if (pNode.type == PowerNodeType::Generator) {
                pNode.isPowered = pNode.isTurnedOnByUser;
            }
        }

        m_pImpl->activeGrids[newGrid.gridId] = newGrid;
    }
}

// ============================================================================
// SECTION 5: CONVEYOR BELT LOGIC & ITEM ROUTING
// ============================================================================

uint32_t FactorySystem::CreateConveyor(const Vector3D& startPos, const Vector3D& endPos, uint32_t tier) {
    uint32_t id = m_pImpl->GetNextId();
    ConveyorNode belt;
    belt.instanceId = id;
    belt.position = startPos;
    belt.forward = (endPos - startPos).Normalized();
    belt.length = (endPos - startPos).Length();
    belt.tier = std::clamp(tier, 1u, 3u);
    belt.nextNodeId = 0;
    belt.prevNodeId = 0;
    
    m_pImpl->conveyors[id] = belt;
    return id;
}

bool FactorySystem::AddItemToConveyor(uint32_t conveyorId, uint32_t itemId, uint32_t qty) {
    auto it = m_pImpl->conveyors.find(conveyorId);
    if (it == m_pImpl->conveyors.end()) return false;

    // Проверка, есть ли место на старте ленты
    for (const auto& item : it->second.itemsOnBelt) {
        if (item.positionOffset < 0.1f) return false; // Затор (Bottleneck) на входе
    }

    BeltItem newItem;
    newItem.itemId = itemId;
    newItem.quantity = qty;
    newItem.positionOffset = 0.0f; // Старт ленты
    
    it->second.itemsOnBelt.push_back(newItem);
    return true;
}

void FactorySystem::UpdateConveyorsLogic(float dt) {
    for (auto& [id, belt] : m_pImpl->conveyors) {
        float speed = (belt.tier == 1) ? FactoryConfig::BELT_SPEED_TIER_1 : 
                      (belt.tier == 2) ? FactoryConfig::BELT_SPEED_TIER_2 : 9.0f; // Tier 3
        
        float progressDelta = (speed * dt) / belt.length; // Нормализованное движение (0.0 - 1.0)

        // Движение предметов с проверкой коллизий друг с другом
        for (size_t i = 0; i < belt.itemsOnBelt.size(); ++i) {
            BeltItem& currentItem = belt.itemsOnBelt[i];
            
            // Если впереди есть другой предмет на той же ленте, мы не можем сквозь него пройти
            float maxForward = 1.0f;
            if (i > 0) { // Предметы сортированы по offset по убыванию
                maxForward = belt.itemsOnBelt[i - 1].positionOffset - 0.1f; // 0.1f = физический размер предмета
            }

            currentItem.positionOffset += progressDelta;
            
            if (currentItem.positionOffset > maxForward) {
                currentItem.positionOffset = maxForward; // Уперлись в предмет впереди (Затор)
            }
        }

        // Передача предмета на следующую ленту или в сундук, если он достиг конца (1.0f)
        if (!belt.itemsOnBelt.empty() && belt.itemsOnBelt.front().positionOffset >= 1.0f) {
            if (belt.nextNodeId != 0) {
                // Пытаемся закинуть на следующую ленту
                if (AddItemToConveyor(belt.nextNodeId, belt.itemsOnBelt.front().itemId, belt.itemsOnBelt.front().quantity)) {
                    belt.itemsOnBelt.erase(belt.itemsOnBelt.begin()); // Успешно передано
                }
                // Иначе предмет ждет (Затор распространяется назад)
            } else {
                // Лента никуда не подключена, предметы падают на землю
                Vector3D dropPos = belt.position + (belt.forward * belt.length);
                // LootSystem::DropItemEntity(belt.itemsOnBelt.front().itemId, dropPos);
                belt.itemsOnBelt.erase(belt.itemsOnBelt.begin());
            }
        }
    }
}

// ============================================================================
// SECTION 6: AUTOMATED EXTRACTORS (DRILLS, PURIFIERS, PLANTERS)
// ============================================================================

uint32_t FactorySystem::CreateExtractor(uint32_t powerNodeId, uint32_t outputBeltId, uint32_t producedItem, float cycleTime) {
    uint32_t id = m_pImpl->GetNextId();
    ExtractorNode ext;
    ext.instanceId = id;
    ext.powerNodeId = powerNodeId;
    ext.outputBeltId = outputBeltId;
    ext.producedItemId = producedItem;
    ext.productionYield = 1;
    ext.cycleTimeSeconds = cycleTime;
    ext.currentCycleProgress = 0.0f;
    ext.requiresWater = false;
    ext.isOperating = false;

    m_pImpl->extractors[id] = ext;
    return id;
}

void FactorySystem::UpdateExtractorsLogic(float dt) {
    for (auto& [id, ext] : m_pImpl->extractors) {
        // Проверка питания
        auto pNodeIt = m_pImpl->powerNodes.find(ext.powerNodeId);
        if (pNodeIt == m_pImpl->powerNodes.end() || !pNodeIt->second.isPowered) {
            ext.isOperating = false;
            continue;
        }

        ext.isOperating = true;
        ext.currentCycleProgress += dt;

        if (ext.currentCycleProgress >= ext.cycleTimeSeconds) {
            // Цикл завершен, пытаемся выдать продукт
            bool outputSuccess = false;
            
            if (ext.outputBeltId != 0) {
                outputSuccess = AddItemToConveyor(ext.outputBeltId, ext.producedItemId, ext.productionYield);
            } else {
                // Если ленты нет, складируем во внутренний инвентарь или выбрасываем
                // LootSystem::DropItemEntity(ext.producedItemId, pNodeIt->second.position);
                outputSuccess = true;
            }

            if (outputSuccess) {
                ext.currentCycleProgress = 0.0f; // Перезапуск цикла
            } else {
                // Буфер переполнен, экстрактор простаивает
                ext.currentCycleProgress = ext.cycleTimeSeconds;
                ext.isOperating = false;
            }
        }
    }
}

// ============================================================================
// SECTION 7: BASE DEFENSE (AUTOMATED TURRETS)
// ============================================================================

uint32_t FactorySystem::CreateTurret(uint32_t powerNodeId, const Vector3D& pos, float fireRate, uint32_t ammoId) {
    uint32_t id = m_pImpl->GetNextId();
    AutomatedTurret turret;
    turret.instanceId = id;
    turret.powerNodeId = powerNodeId;
    turret.position = pos;
    turret.currentYaw = 0.0f;
    turret.currentPitch = 0.0f;
    turret.targetEntityId = 0;
    turret.fireRateCooldown = fireRate;
    turret.fireRateTimer = 0.0f;
    turret.ammoItemId = ammoId;
    turret.ammoLoaded = 500; // Для тестов предзаряжена
    turret.isOperational = false;

    m_pImpl->turrets[id] = turret;
    return id;
}

void FactorySystem::UpdateTurretsLogic(float dt) {
    for (auto& [id, turret] : m_pImpl->turrets) {
        auto pNodeIt = m_pImpl->powerNodes.find(turret.powerNodeId);
        if (pNodeIt == m_pImpl->powerNodes.end() || !pNodeIt->second.isPowered) {
            turret.isOperational = false;
            continue;
        }

        if (turret.ammoLoaded == 0) {
            turret.isOperational = false; // Требуется перезарядка игроком или конвейером
            continue;
        }

        turret.isOperational = true;
        
        // 1. Поиск цели (Радар)
        if (turret.targetEntityId == 0) {
            // В реальном движке: std::vector<Entity*> enemies = SpatialGrid::FindEnemiesInRange(turret.position, SCAN_RADIUS);
            // Если нашли - turret.targetEntityId = enemies[0]->GetId();
            
            // Имитация сканирования (медленное вращение башни)
            turret.currentYaw += 45.0f * dt; 
            if (turret.currentYaw > 360.0f) turret.currentYaw -= 360.0f;
            continue;
        }

        // 2. Наведение на цель и баллистическое упреждение
        // Vector3D targetPos = EntityManager::GetEntityPosition(turret.targetEntityId);
        Vector3D targetPos(10, 0, 10); // Заглушка
        Vector3D toTarget = targetPos - turret.position;
        float distance = toTarget.Length();

        if (distance > FactoryConfig::TURRET_SCAN_RADIUS) {
            turret.targetEntityId = 0; // Цель ушла из радиуса поражения
            continue;
        }

        // Расчет углов Yaw и Pitch для ствола
        toTarget = toTarget.Normalized();
        float targetYaw = std::atan2(toTarget.x, toTarget.z) * (180.0f / 3.14159f);
        float targetPitch = std::asin(toTarget.y) * (180.0f / 3.14159f);

        // Плавная интерполяция поворота ствола (Servo Motors)
        turret.currentYaw = std::lerp(turret.currentYaw, targetYaw, dt * 10.0f);
        turret.currentPitch = std::lerp(turret.currentPitch, targetPitch, dt * 10.0f);

        // 3. Стрельба
        turret.fireRateTimer -= dt;
        if (turret.fireRateTimer <= 0.0f) {
            // Проверка, что ствол направлен примерно на цель (погрешность наведения < 5 градусов)
            if (std::abs(turret.currentYaw - targetYaw) < 5.0f) {
                // Выстрел
                turret.ammoLoaded--;
                turret.fireRateTimer = turret.fireRateCooldown;
                
                Platform::Log("[FACTORY DEFENSE]: Турель ID " + std::to_string(id) + " открыла огонь! Очередь: 1 патрон.");
                
                // DamageSystem::ApplyDamage(turret.targetEntityId, 15.0f);
                // AudioSystem::PlaySound3D("sounds/turret_fire.wav", turret.position);
            }
        }
    }
}

// ============================================================================
// SECTION 8: GLOBAL UPDATE TICK ENGINE
// ============================================================================

void FactorySystem::UpdateTick(float deltaTime) {
    m_pImpl->tickAccumulator += deltaTime;

    // Выполнение симуляции фиксированными шагами для детерминизма конвейеров
    while (m_pImpl->tickAccumulator >= FactoryConfig::GLOBAL_TICK_RATE) {
        float fixedDt = FactoryConfig::GLOBAL_TICK_RATE;

        // 1. Физика электросетей (Потребление/Разрядка батарей)
        for (auto& [gridId, grid] : m_pImpl->activeGrids) {
            if (grid.totalConsumption > grid.totalGeneration) {
                float deficit = grid.totalConsumption - grid.totalGeneration;
                
                // Если есть батареи, сосем энергию из них
                if (grid.batteryStorage > 0.0f) {
                    float drainAmount = deficit * fixedDt;
                    grid.batteryStorage -= drainAmount;
                    
                    // Обновляем состояние физических узлов батарей
                    for (uint32_t nId : grid.nodeIds) {
                        PowerNode& node = m_pImpl->powerNodes[nId];
                        if (node.type == PowerNodeType::Battery) {
                            node.currentCharge -= drainAmount * (node.maxCapacity / grid.batteryMaxCapacity); // Пропорциональный разряд
                            if (node.currentCharge < 0.0f) node.currentCharge = 0.0f;
                        }
                    }

                    if (grid.batteryStorage <= 0.0f) {
                        grid.batteryStorage = 0.0f;
                        RecalculatePowerGrids(); // Блэкаут - пересчет сети
                    }
                }
            } else if (grid.totalGeneration > grid.totalConsumption && grid.batteryStorage < grid.batteryMaxCapacity) {
                // Зарядка батарей излишками
                float surplus = grid.totalGeneration - grid.totalConsumption;
                grid.batteryStorage += surplus * fixedDt;
                if (grid.batteryStorage > grid.batteryMaxCapacity) grid.batteryStorage = grid.batteryMaxCapacity;
                
                for (uint32_t nId : grid.nodeIds) {
                    PowerNode& node = m_pImpl->powerNodes[nId];
                    if (node.type == PowerNodeType::Battery) {
                        node.currentCharge += surplus * fixedDt * (node.maxCapacity / grid.batteryMaxCapacity);
                        if (node.currentCharge > node.maxCapacity) node.currentCharge = node.maxCapacity;
                    }
                }
            }
        }

        // 2. Физика конвейеров
        UpdateConveyorsLogic(fixedDt);

        // 3. Логика Экстракторов и Ферм
        UpdateExtractorsLogic(fixedDt);

        // 4. ИИ оборонительных сооружений
        UpdateTurretsLogic(fixedDt);

        m_pImpl->tickAccumulator -= fixedDt;
    }
}

// ============================================================================
// SECTION 9: BINARY SERIALIZATION (SAVING THE ENTIRE FACTORY STATE)
// ============================================================================

uint32_t FactorySystem::CalculateChecksum(const std::vector<uint8_t>& buffer) const noexcept {
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : buffer) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

std::vector<uint8_t> FactorySystem::SerializeToBinary() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(1024 * 64); // 64 KB резерв под массивную базу

    const uint8_t* magicPtr = reinterpret_cast<const uint8_t*>(&FactoryConfig::FACTORY_SAVE_MAGIC);
    buffer.insert(buffer.end(), magicPtr, magicPtr + sizeof(uint32_t));

    const uint8_t* verPtr = reinterpret_cast<const uint8_t*>(&FactoryConfig::FACTORY_SAVE_VERSION);
    buffer.insert(buffer.end(), verPtr, verPtr + sizeof(uint32_t));

    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_pImpl->nextInstanceId), reinterpret_cast<const uint8_t*>(&m_pImpl->nextInstanceId) + sizeof(uint32_t));

    // 1. Сериализация Узлов питания (Power Nodes)
    uint32_t pNodeCount = static_cast<uint32_t>(m_pImpl->powerNodes.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&pNodeCount), reinterpret_cast<const uint8_t*>(&pNodeCount) + sizeof(uint32_t));
    
    for (const auto& [id, node] : m_pImpl->powerNodes) {
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&node.instanceId), reinterpret_cast<const uint8_t*>(&node.instanceId) + sizeof(uint32_t));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&node.type), reinterpret_cast<const uint8_t*>(&node.type) + sizeof(PowerNodeType));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&node.position), reinterpret_cast<const uint8_t*>(&node.position) + sizeof(Vector3D));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&node.currentCharge), reinterpret_cast<const uint8_t*>(&node.currentCharge) + sizeof(float));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&node.isTurnedOnByUser), reinterpret_cast<const uint8_t*>(&node.isTurnedOnByUser) + sizeof(bool));
        
        uint32_t wireCount = static_cast<uint32_t>(node.connectedWires.size());
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&wireCount), reinterpret_cast<const uint8_t*>(&wireCount) + sizeof(uint32_t));
        for (uint32_t wireId : node.connectedWires) {
            buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&wireId), reinterpret_cast<const uint8_t*>(&wireId) + sizeof(uint32_t));
        }
    }

    // 2. Сериализация Конвейеров и Предметов на них
    uint32_t convCount = static_cast<uint32_t>(m_pImpl->conveyors.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&convCount), reinterpret_cast<const uint8_t*>(&convCount) + sizeof(uint32_t));
    
    for (const auto& [id, belt] : m_pImpl->conveyors) {
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&belt.instanceId), reinterpret_cast<const uint8_t*>(&belt.instanceId) + sizeof(uint32_t));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&belt.position), reinterpret_cast<const uint8_t*>(&belt.position) + sizeof(Vector3D));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&belt.forward), reinterpret_cast<const uint8_t*>(&belt.forward) + sizeof(Vector3D));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&belt.tier), reinterpret_cast<const uint8_t*>(&belt.tier) + sizeof(uint32_t));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&belt.nextNodeId), reinterpret_cast<const uint8_t*>(&belt.nextNodeId) + sizeof(uint32_t));
        
        uint32_t itemCount = static_cast<uint32_t>(belt.itemsOnBelt.size());
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&itemCount), reinterpret_cast<const uint8_t*>(&itemCount) + sizeof(uint32_t));
        for (const auto& item : belt.itemsOnBelt) {
            buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&item), reinterpret_cast<const uint8_t*>(&item) + sizeof(BeltItem));
        }
    }

    // 3. Сериализация Турелей
    uint32_t turretCount = static_cast<uint32_t>(m_pImpl->turrets.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&turretCount), reinterpret_cast<const uint8_t*>(&turretCount) + sizeof(uint32_t));
    
    for (const auto& [id, turret] : m_pImpl->turrets) {
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&turret), reinterpret_cast<const uint8_t*>(&turret) + sizeof(AutomatedTurret));
    }

    // CRC32 Validation Checksum
    uint32_t checksum = CalculateChecksum(buffer);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&checksum), reinterpret_cast<const uint8_t*>(&checksum) + sizeof(uint32_t));

    Platform::Log("[FACTORY SERIALIZE]: Состояние всех автоматизированных баз сохранено (" + std::to_string(buffer.size() / 1024) + " KB).");
    return buffer;
}

bool FactorySystem::DeserializeFromBinary(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(uint32_t) * 4) return false;

    size_t payloadSize = buffer.size() - sizeof(uint32_t);
    std::vector<uint8_t> payloadData(buffer.begin(), buffer.begin() + payloadSize);
    uint32_t expectedChecksum = CalculateChecksum(payloadData);

    uint32_t storedChecksum = 0;
    std::memcpy(&storedChecksum, buffer.data() + payloadSize, sizeof(uint32_t));

    if (expectedChecksum != storedChecksum) {
        Platform::Log("[FACTORY DESERIALIZE ERROR]: Искажение бинарного дампа (CRC32 Mismatch). Сохранение базы повреждено.");
        return false;
    }

    size_t cursor = 0;
    uint32_t magic;
    std::memcpy(&magic, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    if (magic != FactoryConfig::FACTORY_SAVE_MAGIC) return false;

    uint32_t version;
    std::memcpy(&version, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

    std::memcpy(&m_pImpl->nextInstanceId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

    // Очистка текущего стейта
    m_pImpl->powerNodes.clear();
    m_pImpl->conveyors.clear();
    m_pImpl->turrets.clear();

    // 1. Загрузка Power Nodes
    uint32_t pNodeCount;
    std::memcpy(&pNodeCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    
    for (uint32_t i = 0; i < pNodeCount; ++i) {
        PowerNode node;
        std::memcpy(&node.instanceId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        std::memcpy(&node.type, buffer.data() + cursor, sizeof(PowerNodeType)); cursor += sizeof(PowerNodeType);
        std::memcpy(&node.position, buffer.data() + cursor, sizeof(Vector3D)); cursor += sizeof(Vector3D);
        std::memcpy(&node.currentCharge, buffer.data() + cursor, sizeof(float)); cursor += sizeof(float);
        std::memcpy(&node.isTurnedOnByUser, buffer.data() + cursor, sizeof(bool)); cursor += sizeof(bool);
        
        // Восстановление генерации/потребления по типу
        node.powerGeneration = (node.type == PowerNodeType::Generator) ? 50.0f : 0.0f;
        node.powerConsumption = (node.type == PowerNodeType::Consumer) ? 10.0f : 0.0f;
        node.maxCapacity = (node.type == PowerNodeType::Battery) ? 10000.0f : 0.0f;

        uint32_t wireCount;
        std::memcpy(&wireCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        for (uint32_t j = 0; j < wireCount; ++j) {
            uint32_t wireId;
            std::memcpy(&wireId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
            node.connectedWires.push_back(wireId);
        }
        m_pImpl->powerNodes[node.instanceId] = node;
    }

    // 2. Загрузка Conveyors
    uint32_t convCount;
    std::memcpy(&convCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    
    for (uint32_t i = 0; i < convCount; ++i) {
        ConveyorNode belt;
        std::memcpy(&belt.instanceId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        std::memcpy(&belt.position, buffer.data() + cursor, sizeof(Vector3D)); cursor += sizeof(Vector3D);
        std::memcpy(&belt.forward, buffer.data() + cursor, sizeof(Vector3D)); cursor += sizeof(Vector3D);
        std::memcpy(&belt.tier, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        std::memcpy(&belt.nextNodeId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        belt.length = 2.0f; // Дефолтная длина куска ленты

        uint32_t itemCount;
        std::memcpy(&itemCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        for (uint32_t j = 0; j < itemCount; ++j) {
            BeltItem item;
            std::memcpy(&item, buffer.data() + cursor, sizeof(BeltItem)); cursor += sizeof(BeltItem);
            belt.itemsOnBelt.push_back(item);
        }
        m_pImpl->conveyors[belt.instanceId] = belt;
    }

    // 3. Загрузка Турелей
    uint32_t turretCount;
    std::memcpy(&turretCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    
    for (uint32_t i = 0; i < turretCount; ++i) {
        AutomatedTurret turret;
        std::memcpy(&turret, buffer.data() + cursor, sizeof(AutomatedTurret)); cursor += sizeof(AutomatedTurret);
        m_pImpl->turrets[turret.instanceId] = turret;
    }

    // Восстанавливаем графы сетей после загрузки узлов
    RecalculatePowerGrids();
    
    Platform::Log("[FACTORY DESERIALIZE]: Успешное восстановление состояния автоматизированных баз.");
    return true;
}

} // namespace Centralia