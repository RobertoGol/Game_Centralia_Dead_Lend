#include "gameplay/CraftingManager.hpp"
#include "gameplay/Player.hpp"
#include "platform/Platform.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <random>

namespace Centralia {

// ============================================================================
// CONSTANTS & MACROS FOR CRAFTING SYSTEM
// ============================================================================

namespace {
    constexpr uint32_t CRAFTING_SAVE_MAGIC = 0x43524654; // "CRFT"
    constexpr uint32_t CRAFTING_SAVE_VERSION = 3;
    constexpr size_t MAX_SIMULTANEOUS_CRAFTING_TASKS = 5;

    const char* WorkstationToString(CraftingStation type) noexcept {
        switch (type) {
            case CraftingStation::None:           return "Крафт в руках (Handcraft)";
            case CraftingStation::Campfire:       return "Походный костер (Campfire)";
            case CraftingStation::ChemistryLab:   return "Химическая лаборатория (Chem Lab)";
            case CraftingStation::AmmoPress:      return "Пресс для боеприпасов (Ammo Press)";
            case CraftingStation::MetalLathe:     return "Токарный станок (Metal Lathe)";
            case CraftingStation::ElectronicsDesk:return "Стол электроники (Electronics Desk)";
            default:                              return "Неизвестная станция";
        }
    }

    const char* ItemQualityToString(ItemQuality q) noexcept {
        switch (q) {
            case ItemQuality::Poor:       return "Бракованное (Poor)";
            case ItemQuality::Standard:   return "Стандартное (Standard)";
            case ItemQuality::HighGrade:  return "Высококачественное (High-Grade)";
            case ItemQuality::Masterwork: return "Шедевр (Masterwork)";
            default:                      return "Неизвестно";
        }
    }
}

// ============================================================================
// SECTION 1: CONSTRUCTOR, DESTRUCTOR & INITIALIZATION
// ============================================================================

CraftingManager::CraftingManager()
    : m_totalBlueprintsLoaded(0),
      m_totalItemsCrafted(0),
      m_activeStation(CraftingStation::None),
      m_playerCraftingSkillLevel(1)
{
    m_recipeDatabase.clear();
    m_unlockedBlueprints.clear();
    m_activeTasks.clear();
    
    // Инициализация генератора случайных чисел для расчета качества предметов
    std::random_device rd;
    m_rngEngine.seed(rd());

    InitializeRecipeDatabase();
    Platform::Log("[CRAFTING SYSTEM]: Подсистема создания предметов (CraftingManager) успешно инициализирована.");
}

CraftingManager::~CraftingManager() {
    m_recipeDatabase.clear();
    m_unlockedBlueprints.clear();
    m_activeTasks.clear();
    Platform::Log("[CRAFTING SYSTEM]: Буферы менеджера крафта штатно выгружены из памяти.");
}

// ============================================================================
// SECTION 2: MASSIVE RECIPE DATABASE (BLUEPRINTS)
// ============================================================================

void CraftingManager::InitializeRecipeDatabase() {
    m_recipeDatabase.clear();

    // ------------------------------------------------------------------------
    // CATEGORY: СУРВАЙВАЛ (ЕДА И ВОДА) - Крафт на костре
    // ------------------------------------------------------------------------

    CraftRecipe rCookedMeat;
    rCookedMeat.recipeId = 101;
    rCookedMeat.category = CraftCategory::Survival;
    rCookedMeat.requiredStation = CraftingStation::Campfire;
    rCookedMeat.recipeName = "Жареное мясо рад-оленя";
    rCookedMeat.outputItemId = 3001; // ID жареного мяса
    rCookedMeat.outputQuantity = 1;
    rCookedMeat.craftTimeSeconds = 15.0f;
    rCookedMeat.requiredSkillLevel = 1;
    rCookedMeat.xpGranted = 5;
    rCookedMeat.isDefaultUnlocked = true;
    rCookedMeat.materials = { {3000, 1}, {2001, 2} }; // Сырое мясо, Древесина
    m_recipeDatabase[rCookedMeat.recipeId] = rCookedMeat;

    CraftRecipe rPurifiedWater;
    rPurifiedWater.recipeId = 102;
    rPurifiedWater.category = CraftCategory::Survival;
    rPurifiedWater.requiredStation = CraftingStation::Campfire;
    rPurifiedWater.recipeName = "Очищенная кипяченая вода";
    rPurifiedWater.outputItemId = 3005; 
    rPurifiedWater.outputQuantity = 2;
    rPurifiedWater.craftTimeSeconds = 10.0f;
    rPurifiedWater.requiredSkillLevel = 1;
    rPurifiedWater.xpGranted = 3;
    rPurifiedWater.isDefaultUnlocked = true;
    rPurifiedWater.materials = { {3004, 2}, {2001, 1} }; // Грязная вода, Древесина
    m_recipeDatabase[rPurifiedWater.recipeId] = rPurifiedWater;

    // ------------------------------------------------------------------------
    // CATEGORY: МЕДИКАМЕНТЫ И ХИМИЯ - Крафт на хим. лаборатории
    // ------------------------------------------------------------------------

    CraftRecipe rBandage;
    rBandage.recipeId = 201;
    rBandage.category = CraftCategory::Medical;
    rBandage.requiredStation = CraftingStation::None; // Можно скрафтить в руках
    rBandage.recipeName = "Стерильный бинт";
    rBandage.outputItemId = 4001; 
    rBandage.outputQuantity = 1;
    rBandage.craftTimeSeconds = 5.0f;
    rBandage.requiredSkillLevel = 1;
    rBandage.xpGranted = 10;
    rBandage.isDefaultUnlocked = true;
    rBandage.materials = { {4000, 2}, {4005, 1} }; // Грязная ткань, Антисептик
    m_recipeDatabase[rBandage.recipeId] = rBandage;

    CraftRecipe rStimpak;
    rStimpak.recipeId = 202;
    rStimpak.category = CraftCategory::Medical;
    rStimpak.requiredStation = CraftingStation::ChemistryLab;
    rStimpak.recipeName = "Стимпак (Медицинский инъектор)";
    rStimpak.outputItemId = 4002;
    rStimpak.outputQuantity = 1;
    rStimpak.craftTimeSeconds = 30.0f;
    rStimpak.requiredSkillLevel = 3;
    rStimpak.xpGranted = 45;
    rStimpak.isDefaultUnlocked = false; // Требуется найти чертеж
    rStimpak.materials = { {4005, 2}, {2002, 1}, {4010, 1} }; // Антисептик, Сталь, Плазма крови
    m_recipeDatabase[rStimpak.recipeId] = rStimpak;

    CraftRecipe rRadAway;
    rRadAway.recipeId = 203;
    rRadAway.category = CraftCategory::Medical;
    rRadAway.requiredStation = CraftingStation::ChemistryLab;
    rRadAway.recipeName = "Антирадин (Детоксикант)";
    rRadAway.outputItemId = 4003;
    rRadAway.outputQuantity = 1;
    rRadAway.craftTimeSeconds = 45.0f;
    rRadAway.requiredSkillLevel = 4;
    rRadAway.xpGranted = 60;
    rRadAway.isDefaultUnlocked = false;
    rRadAway.materials = { {3005, 2}, {4015, 3}, {4020, 1} }; // Очищенная вода, Светящийся гриб, Пластик
    m_recipeDatabase[rRadAway.recipeId] = rRadAway;

    // ------------------------------------------------------------------------
    // CATEGORY: БОЕПРИПАСЫ И ВЗРЫВЧАТКА - Крафт на прессе
    // ------------------------------------------------------------------------

    CraftRecipe rAmmo9mm;
    rAmmo9mm.recipeId = 301;
    rAmmo9mm.category = CraftCategory::Ammunition;
    rAmmo9mm.requiredStation = CraftingStation::AmmoPress;
    rAmmo9mm.recipeName = "Патроны 9x19mm Parabellum (Коробка 24 шт)";
    rAmmo9mm.outputItemId = 600; 
    rAmmo9mm.outputQuantity = 24;
    rAmmo9mm.craftTimeSeconds = 20.0f;
    rAmmo9mm.requiredSkillLevel = 2;
    rAmmo9mm.xpGranted = 25;
    rAmmo9mm.isDefaultUnlocked = true;
    rAmmo9mm.materials = { {5001, 8}, {5002, 3}, {5003, 1} }; // Свинец, Порох, Медь (для гильз)
    m_recipeDatabase[rAmmo9mm.recipeId] = rAmmo9mm;

    CraftRecipe rAmmo556;
    rAmmo556.recipeId = 302;
    rAmmo556.category = CraftCategory::Ammunition;
    rAmmo556.requiredStation = CraftingStation::AmmoPress;
    rAmmo556.recipeName = "Патроны 5.56x45mm NATO (Коробка 30 шт)";
    rAmmo556.outputItemId = 601;
    rAmmo556.outputQuantity = 30;
    rAmmo556.craftTimeSeconds = 35.0f;
    rAmmo556.requiredSkillLevel = 3;
    rAmmo556.xpGranted = 40;
    rAmmo556.isDefaultUnlocked = false;
    rAmmo556.materials = { {5001, 12}, {5002, 6}, {5003, 2} };
    m_recipeDatabase[rAmmo556.recipeId] = rAmmo556;

    CraftRecipe rFragGrenade;
    rFragGrenade.recipeId = 303;
    rFragGrenade.category = CraftCategory::Ammunition;
    rFragGrenade.requiredStation = CraftingStation::ChemistryLab;
    rFragGrenade.recipeName = "Осколочная граната Ф-1 (Самодельная)";
    rFragGrenade.outputItemId = 610;
    rFragGrenade.outputQuantity = 1;
    rFragGrenade.craftTimeSeconds = 25.0f;
    rFragGrenade.requiredSkillLevel = 3;
    rFragGrenade.xpGranted = 35;
    rFragGrenade.isDefaultUnlocked = true;
    rFragGrenade.materials = { {2002, 5}, {5002, 8}, {2010, 1} }; // Сталь, Порох, Пружина
    m_recipeDatabase[rFragGrenade.recipeId] = rFragGrenade;

    // ------------------------------------------------------------------------
    // CATEGORY: СТРОИТЕЛЬСТВО БАЗЫ (BASE BUILDING) - Крафт на токарном станке
    // ------------------------------------------------------------------------

    CraftRecipe rWoodWall;
    rWoodWall.recipeId = 401;
    rWoodWall.category = CraftCategory::BaseBuilding;
    rWoodWall.requiredStation = CraftingStation::None;
    rWoodWall.recipeName = "Деревянная стена с бойницей";
    rWoodWall.outputItemId = 8001;
    rWoodWall.outputQuantity = 1;
    rWoodWall.craftTimeSeconds = 10.0f;
    rWoodWall.requiredSkillLevel = 1;
    rWoodWall.xpGranted = 15;
    rWoodWall.isDefaultUnlocked = true;
    rWoodWall.materials = { {2001, 15}, {2002, 2} }; // Древесина, Сталь (гвозди)
    m_recipeDatabase[rWoodWall.recipeId] = rWoodWall;

    CraftRecipe rGenerator;
    rGenerator.recipeId = 402;
    rGenerator.category = CraftCategory::BaseBuilding;
    rGenerator.requiredStation = CraftingStation::MetalLathe;
    rGenerator.recipeName = "Малый бензиновый генератор (5 кВт)";
    rGenerator.outputItemId = 8005;
    rGenerator.outputQuantity = 1;
    rGenerator.craftTimeSeconds = 120.0f;
    rGenerator.requiredSkillLevel = 4;
    rGenerator.xpGranted = 200;
    rGenerator.isDefaultUnlocked = false;
    rGenerator.materials = { {2002, 45}, {2003, 10}, {2012, 5}, {2015, 8} }; // Сталь, Ядра, Медные провода, Шестеренки
    m_recipeDatabase[rGenerator.recipeId] = rGenerator;

    m_totalBlueprintsLoaded = static_cast<uint32_t>(m_recipeDatabase.size());
    
    // Инициализация разблокированных по умолчанию чертежей
    for (const auto& [id, recipe] : m_recipeDatabase) {
        if (recipe.isDefaultUnlocked) {
            m_unlockedBlueprints.insert(id);
        }
    }

    Platform::Log("[CRAFTING SYSTEM]: База данных чертежей сформирована. Загружено рецептов: " + std::to_string(m_totalBlueprintsLoaded));
}

// ============================================================================
// SECTION 3: BLUEPRINT UNLOCKING & AVAILABILITY VERIFICATION
// ============================================================================

bool CraftingManager::UnlockBlueprint(uint32_t recipeId) {
    auto it = m_recipeDatabase.find(recipeId);
    if (it == m_recipeDatabase.end()) {
        Platform::Log("[CRAFTING ERROR]: Попытка разблокировать несуществующий чертеж ID: " + std::to_string(recipeId));
        return false;
    }

    if (m_unlockedBlueprints.find(recipeId) != m_unlockedBlueprints.end()) {
        Platform::Log("[CRAFTING INFO]: Чертеж '" + it->second.recipeName + "' уже был изучен ранее.");
        return false;
    }

    m_unlockedBlueprints.insert(recipeId);
    Platform::Log("[CRAFTING UNLOCK]: Новый чертеж изучен! Доступен крафт: " + it->second.recipeName);
    return true;
}

bool CraftingManager::IsBlueprintUnlocked(uint32_t recipeId) const noexcept {
    return m_unlockedBlueprints.find(recipeId) != m_unlockedBlueprints.end();
}

std::vector<CraftRecipe> CraftingManager::GetAvailableRecipes(CraftCategory filterCategory, CraftingStation currentStation) const {
    std::vector<CraftRecipe> available;
    
    for (uint32_t id : m_unlockedBlueprints) {
        const auto& recipe = m_recipeDatabase.at(id);
        
        // Фильтрация по категории (если не выбрана "Все")
        if (filterCategory != CraftCategory::All && recipe.category != filterCategory) {
            continue;
        }

        // Фильтрация по доступному станку
        if (recipe.requiredStation != CraftingStation::None && recipe.requiredStation != currentStation) {
            continue;
        }

        available.push_back(recipe);
    }

    return available;
}

void CraftingManager::SetPlayerCraftingSkill(uint32_t skillLevel) noexcept {
    m_playerCraftingSkillLevel = std::max(1u, skillLevel);
}

void CraftingManager::SetActiveCraftingStation(CraftingStation station) noexcept {
    m_activeStation = station;
    Platform::Log(std::string("[CRAFTING SYSTEM]: Игрок использует станцию: ") + WorkstationToString(station));
}

// ============================================================================
// SECTION 4: INVENTORY AUDIT & BATCH CRAFTING INITIATION
// ============================================================================

bool CraftingManager::HasRequiredMaterials(const Player& player, const CraftRecipe& recipe, uint32_t batchSize) const {
    const auto& inventory = player.GetInventory();

    for (const auto& [materialId, requiredQty] : recipe.materials) {
        uint32_t totalNeeded = requiredQty * batchSize;
        uint32_t totalFound = 0;

        for (const auto& item : inventory) {
            if (item.id == materialId) {
                totalFound += item.quantity;
            }
        }

        if (totalFound < totalNeeded) {
            Platform::Log("[CRAFT ERROR]: Нехватка компонента ID " + std::to_string(materialId) + 
                          ". Требуется: " + std::to_string(totalNeeded) + ", В наличии: " + std::to_string(totalFound));
            return false;
        }
    }

    return true;
}

bool CraftingManager::ConsumeMaterials(Player& player, const CraftRecipe& recipe, uint32_t batchSize) {
    const auto& inventory = player.GetInventory();

    // Повторная верификация перед жестким списанием
    if (!HasRequiredMaterials(player, recipe, batchSize)) {
        return false;
    }

    for (const auto& [materialId, requiredQty] : recipe.materials) {
        uint32_t pendingDeduction = requiredQty * batchSize;

        for (size_t i = 0; i < inventory.size(); ++i) {
            if (inventory[i].id == materialId) {
                uint32_t takeCount = std::min(pendingDeduction, static_cast<uint32_t>(inventory[i].quantity));
                player.RemoveItem(i, static_cast<uint16_t>(takeCount));
                pendingDeduction -= takeCount;
                if (pendingDeduction == 0) break;
            }
        }
    }

    Platform::Log("[CRAFT AUDIT]: Расходные материалы успешно списаны из инвентаря игрока (Batch: " + std::to_string(batchSize) + "x).");
    return true;
}

bool CraftingManager::QueueCraftingTask(Player& player, uint32_t recipeId, uint32_t batchSize) {
    if (batchSize == 0) return false;

    if (m_activeTasks.size() >= MAX_SIMULTANEOUS_CRAFTING_TASKS) {
        Platform::Log("[CRAFT QUEUE ERROR]: Достигнут лимит одновременных задач создания (" + std::to_string(MAX_SIMULTANEOUS_CRAFTING_TASKS) + ").");
        return false;
    }

    auto it = m_recipeDatabase.find(recipeId);
    if (it == m_recipeDatabase.end()) return false;

    const CraftRecipe& recipe = it->second;

    if (!IsBlueprintUnlocked(recipeId)) {
        Platform::Log("[CRAFT ERROR]: Чертеж не изучен!");
        return false;
    }

    if (m_playerCraftingSkillLevel < recipe.requiredSkillLevel) {
        Platform::Log("[CRAFT ERROR]: Недостаточный уровень навыка. Требуется: " + std::to_string(recipe.requiredSkillLevel));
        return false;
    }

    if (recipe.requiredStation != CraftingStation::None && m_activeStation != recipe.requiredStation) {
        Platform::Log(std::string("[CRAFT ERROR]: Требуется станция: ") + WorkstationToString(recipe.requiredStation));
        return false;
    }

    if (!ConsumeMaterials(player, recipe, batchSize)) {
        return false;
    }

    CraftingTask newTask;
    newTask.taskId = static_cast<uint32_t>(m_totalItemsCrafted + m_activeTasks.size() + 1);
    newTask.recipeId = recipeId;
    newTask.batchSize = batchSize;
    newTask.totalDurationSeconds = recipe.craftTimeSeconds * batchSize;
    
    // Оптовый крафт дает 15% бонус к скорости
    if (batchSize >= 5) {
        newTask.totalDurationSeconds *= 0.85f;
    }
    
    newTask.progressSeconds = 0.0f;
    newTask.isCompleted = false;

    m_activeTasks.push_back(newTask);
    Platform::Log("[CRAFT QUEUE]: Добавлена задача: " + recipe.recipeName + " (x" + std::to_string(batchSize) + 
                  "). Время выполнения: " + std::to_string(newTask.totalDurationSeconds) + " сек.");

    return true;
}

// ============================================================================
// SECTION 5: ITEM QUALITY RNG & DEGRADATION MECHANICS
// ============================================================================

ItemQuality CraftingManager::CalculateCraftingQuality(const CraftRecipe& recipe) {
    // Вероятность качества зависит от разницы между скиллом игрока и сложностью рецепта
    int skillDelta = static_cast<int>(m_playerCraftingSkillLevel) - static_cast<int>(recipe.requiredSkillLevel);
    
    std::uniform_real_distribution<float> dist(0.0f, 100.0f);
    float roll = dist(m_rngEngine);

    if (skillDelta < 0) {
        // Крафт предмета, который сложнее уровня игрока (в теории блокируется, но если обход)
        return ItemQuality::Poor;
    }

    // Базовые шансы: 10% Брак, 70% Стандарт, 18% Высший сорт, 2% Шедевр
    float chanceMasterwork = 2.0f + (skillDelta * 2.5f);
    float chanceHighGrade = 18.0f + (skillDelta * 5.0f);
    float chancePoor = std::max(0.0f, 10.0f - (skillDelta * 3.0f));

    if (roll < chanceMasterwork) return ItemQuality::Masterwork;
    if (roll < chanceMasterwork + chanceHighGrade) return ItemQuality::HighGrade;
    if (roll > 100.0f - chancePoor) return ItemQuality::Poor;

    return ItemQuality::Standard;
}

// ============================================================================
// SECTION 6: ASYNCHRONOUS TICK UPDATE & TASK COMPLETION
// ============================================================================

void CraftingManager::UpdateTick(float deltaTime, Player& player) {
    if (m_activeTasks.empty()) return;

    // В Centralia мы используем параллельный крафт - все задачи в очереди делаются одновременно (или можно сделать последовательно)
    // Для хардкорного выживания сделаем последовательно: прогресс идет только у первой задачи.
    
    CraftingTask& currentTask = m_activeTasks.front();
    
    if (!currentTask.isCompleted) {
        currentTask.progressSeconds += deltaTime;

        if (currentTask.progressSeconds >= currentTask.totalDurationSeconds) {
            CompleteTask(currentTask, player);
            m_activeTasks.erase(m_activeTasks.begin());
        }
    }
}

void CraftingManager::CompleteTask(CraftingTask& task, Player& player) {
    const CraftRecipe& recipe = m_recipeDatabase.at(task.recipeId);
    
    uint32_t totalOutputQty = recipe.outputQuantity * task.batchSize;
    ItemQuality quality = CalculateCraftingQuality(recipe);

    // Добавление созданных предметов в инвентарь
    player.AddItemToInventory(recipe.outputItemId, static_cast<uint16_t>(totalOutputQty));
    
    // Начисление опыта за крафт
    uint32_t totalXp = recipe.xpGranted * task.batchSize;
    
    // Бонус к опыту за высокое качество
    if (quality == ItemQuality::Masterwork) totalXp = static_cast<uint32_t>(totalXp * 1.5f);
    
    player.AddExperience(totalXp);

    m_totalItemsCrafted += task.batchSize;
    task.isCompleted = true;

    Platform::Log("[CRAFT COMPLETE]: Создано: " + recipe.recipeName + " (x" + std::to_string(totalOutputQty) + 
                  "). Качество: " + ItemQualityToString(quality) + ". Опыт: +" + std::to_string(totalXp));
}

bool CraftingManager::CancelTask(uint32_t taskId, Player& player) {
    auto it = std::find_if(m_activeTasks.begin(), m_activeTasks.end(), 
                           [taskId](const CraftingTask& t) { return t.taskId == taskId; });

    if (it == m_activeTasks.end()) return false;

    const CraftRecipe& recipe = m_recipeDatabase.at(it->recipeId);

    // Расчет возврата ресурсов (штраф за отмену). Чем больше прогресс, тем больше ресурсов сгорает.
    float progressRatio = it->progressSeconds / it->totalDurationSeconds;
    float refundMultiplier = 1.0f - (progressRatio * 0.8f); // Максимум теряется 80% материалов

    for (const auto& [materialId, requiredQty] : recipe.materials) {
        uint32_t totalInvested = requiredQty * it->batchSize;
        uint32_t refundAmount = static_cast<uint32_t>(totalInvested * refundMultiplier);
        
        if (refundAmount > 0) {
            player.AddItemToInventory(materialId, static_cast<uint16_t>(refundAmount));
        }
    }

    Platform::Log("[CRAFT CANCEL]: Задача #" + std::to_string(taskId) + " отменена игроком. Возвращено " + 
                  std::to_string(static_cast<int>(refundMultiplier * 100.0f)) + "% материалов.");
                  
    m_activeTasks.erase(it);
    return true;
}

const std::vector<CraftingTask>& CraftingManager::GetActiveTasks() const noexcept {
    return m_activeTasks;
}

// ============================================================================
// SECTION 7: BINARY SERIALIZATION (GHOST-RAM & DISK SAVING)
// ============================================================================

uint32_t CraftingManager::CalculateChecksum(const std::vector<uint8_t>& buffer) const noexcept {
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : buffer) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

std::vector<uint8_t> CraftingManager::SerializeToBinary() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(1024);

    // Заголовок
    const uint8_t* magicPtr = reinterpret_cast<const uint8_t*>(&CRAFTING_SAVE_MAGIC);
    buffer.insert(buffer.end(), magicPtr, magicPtr + sizeof(uint32_t));

    const uint8_t* verPtr = reinterpret_cast<const uint8_t*>(&CRAFTING_SAVE_VERSION);
    buffer.insert(buffer.end(), verPtr, verPtr + sizeof(uint32_t));

    // Статистика
    const uint8_t* craftedPtr = reinterpret_cast<const uint8_t*>(&m_totalItemsCrafted);
    buffer.insert(buffer.end(), craftedPtr, craftedPtr + sizeof(uint32_t));

    // Сохранение разблокированных чертежей
    uint32_t unlocksCount = static_cast<uint32_t>(m_unlockedBlueprints.size());
    const uint8_t* uCountPtr = reinterpret_cast<const uint8_t*>(&unlocksCount);
    buffer.insert(buffer.end(), uCountPtr, uCountPtr + sizeof(uint32_t));

    for (uint32_t recipeId : m_unlockedBlueprints) {
        const uint8_t* rIdPtr = reinterpret_cast<const uint8_t*>(&recipeId);
        buffer.insert(buffer.end(), rIdPtr, rIdPtr + sizeof(uint32_t));
    }

    // Сохранение активных задач крафта
    uint32_t tasksCount = static_cast<uint32_t>(m_activeTasks.size());
    const uint8_t* tCountPtr = reinterpret_cast<const uint8_t*>(&tasksCount);
    buffer.insert(buffer.end(), tCountPtr, tCountPtr + sizeof(uint32_t));

    for (const auto& task : m_activeTasks) {
        const uint8_t* tPtr = reinterpret_cast<const uint8_t*>(&task);
        buffer.insert(buffer.end(), tPtr, tPtr + sizeof(CraftingTask));
    }

    // Контрольная сумма CRC32
    uint32_t checksum = CalculateChecksum(buffer);
    const uint8_t* chkPtr = reinterpret_cast<const uint8_t*>(&checksum);
    buffer.insert(buffer.end(), chkPtr, chkPtr + sizeof(uint32_t));

    Platform::Log("[CRAFTING SERIALIZE]: Подсистема крафта упакована в бинарный поток (" + std::to_string(buffer.size()) + " байт).");
    return buffer;
}

bool CraftingManager::DeserializeFromBinary(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(uint32_t) * 4) {
        Platform::Log("[CRAFTING LOAD ERROR]: Бинарный поток слишком мал.");
        return false;
    }

    // Проверка контрольной суммы
    size_t payloadSize = buffer.size() - sizeof(uint32_t);
    std::vector<uint8_t> payloadData(buffer.begin(), buffer.begin() + payloadSize);
    uint32_t expectedChecksum = CalculateChecksum(payloadData);

    uint32_t storedChecksum = 0;
    std::memcpy(&storedChecksum, buffer.data() + payloadSize, sizeof(uint32_t));

    if (expectedChecksum != storedChecksum) {
        Platform::Log("[CRAFTING LOAD ERROR]: Нарушена контрольная сумма CRC32 дампа крафта!");
        return false;
    }

    size_t cursor = 0;

    uint32_t magic = 0;
    std::memcpy(&magic, buffer.data() + cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    if (magic != CRAFTING_SAVE_MAGIC) {
        Platform::Log("[CRAFTING LOAD ERROR]: Неверный Magic Identifier.");
        return false;
    }

    uint32_t version = 0;
    std::memcpy(&version, buffer.data() + cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    std::memcpy(&m_totalItemsCrafted, buffer.data() + cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    // Загрузка чертежей
    uint32_t unlocksCount = 0;
    std::memcpy(&unlocksCount, buffer.data() + cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    m_unlockedBlueprints.clear();
    for (uint32_t i = 0; i < unlocksCount; ++i) {
        uint32_t recipeId = 0;
        std::memcpy(&recipeId, buffer.data() + cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);
        m_unlockedBlueprints.insert(recipeId);
    }

    // Загрузка активных задач
    uint32_t tasksCount = 0;
    std::memcpy(&tasksCount, buffer.data() + cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    m_activeTasks.clear();
    for (uint32_t i = 0; i < tasksCount; ++i) {
        CraftingTask task;
        std::memcpy(&task, buffer.data() + cursor, sizeof(CraftingTask));
        cursor += sizeof(CraftingTask);
        m_activeTasks.push_back(task);
    }

    Platform::Log("[CRAFTING DESERIALIZE]: Состояние крафта успешно восстановлено. Изучено чертежей: " + std::to_string(m_unlockedBlueprints.size()));
    return true;
}

} // namespace Centralia