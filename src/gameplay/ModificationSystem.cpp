#include "gameplay/ModificationSystem.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/WeaponSystem.hpp"
#include "gameplay/PowerArmorStateData.hpp"
#include "platform/Platform.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

namespace Centralia {

// ============================================================================
// CONSTANTS & HELPER CONVERSIONS
// ============================================================================

namespace {
    constexpr uint32_t MOD_SAVE_MAGIC = 0x4D4F4453; // "MODS"
    constexpr uint32_t MOD_SAVE_VERSION = 2;
    constexpr float MAX_WORKBENCH_HEAT = 100.0f;
    constexpr float HEAT_COOLDOWN_RATE = 8.5f;

    const char* ModCategoryToString(ModCategory category) noexcept {
        switch (category) {
            case ModCategory::Weapon:      return "Оружие и Баллистика";
            case ModCategory::PowerArmor:  return "Силовая Броня / Экзоскелет";
            case ModCategory::Vehicle:     return "Колесный и Гусеничный Транспорт";
            case ModCategory::TitanChassis:return "Шагающий Титан / Мех";
            case ModCategory::DroneAviation:return "БПЛА и Авиационные Дроны";
            default:                       return "Неизвестная Категория";
        }
    }
}

// ============================================================================
// SECTION 1: CONSTRUCTOR, DESTRUCTOR & LIFECYCLE
// ============================================================================

ModificationSystem::ModificationSystem() 
    : m_workbenchActive(false),
      m_selectedCategory(ModCategory::Weapon),
      m_totalRecipesLoaded(0),
      m_workbenchTierLevel(1),
      m_stationPowerActive(true),
      m_stationHeatLevel(0.0f),
      m_craftingSpeedMultiplier(1.0f),
      m_totalCraftsCompleted(0)
{
    m_recipes.clear();
    m_activeModifications.clear();
    m_craftingQueue.clear();
    m_unlockedPerks.clear();

    InitializeAdvancedWorkbenchCatalog();
    Platform::Log("[MOD SYSTEM]: Полномасштабный производственный комплекс модификаций загружен в ОЗУ.");
}

ModificationSystem::~ModificationSystem() {
    m_recipes.clear();
    m_activeModifications.clear();
    m_craftingQueue.clear();
    m_unlockedPerks.clear();
    Platform::Log("[MOD SYSTEM]: Память верстака и подсистемы модификаций штатно очищена.");
}

// ============================================================================
// SECTION 2: COMPREHENSIVE RECIPE CATALOG INITIALIZATION
// ============================================================================

void ModificationSystem::InitializeAdvancedWorkbenchCatalog() {
    m_recipes.clear();

    // ------------------------------------------------------------------------
    // 1. ОРУЖИЕ: АВТОМАТЫ, ВИНТОВКИ, ДРОБОВИКИ
    // ------------------------------------------------------------------------
    
    // 1001: Тяжелый кованый нарезной ствол 5.45
    ModRecipe heavyBarrel;
    heavyBarrel.recipeId = 1001;
    heavyBarrel.category = ModCategory::Weapon;
    heavyBarrel.slotType = ModSlot::Barrel;
    heavyBarrel.targetId = 101; // Штурмовая винтовка Пустоши
    heavyBarrel.recipeName = "Тяжелый нарезной ствол 415-мм (Хромированный)";
    heavyBarrel.requiredPerkLevel = 1;
    heavyBarrel.requiredWorkbenchTier = 1;
    heavyBarrel.craftTimeSeconds = 6.0f;
    heavyBarrel.materialCosts = { {2001, 4}, {2002, 12}, {2005, 2} }; // Дерево, Сталь, Сплав
    heavyBarrel.statBonusDamage = 14.5f;
    heavyBarrel.statBonusRange = 55.0f;
    heavyBarrel.statBonusRecoil = -0.25f;
    heavyBarrel.statBonusDurabilityMax = 200.0f;
    heavyBarrel.weightImpact = 1.4f;
    m_recipes[heavyBarrel.recipeId] = heavyBarrel;

    // 1002: Сдвоенный штурмовой барабан 60 патронов
    ModRecipe drumMag;
    drumMag.recipeId = 1002;
    drumMag.category = ModCategory::Weapon;
    drumMag.slotType = ModSlot::Magazine;
    drumMag.targetId = 101;
    drumMag.recipeName = "Барабанный магазин высокой емкости (60 патронов)";
    drumMag.requiredPerkLevel = 2;
    drumMag.requiredWorkbenchTier = 2;
    drumMag.craftTimeSeconds = 8.5f;
    drumMag.materialCosts = { {2002, 18}, {2003, 3}, {2006, 4} }; // Сталь, Ядро, Алюминий
    drumMag.statBonusDamage = 0.0f;
    drumMag.statBonusRange = 0.0f;
    drumMag.statBonusRecoil = 0.35f; // Увеличение массы дает отдачу при стрельбе
    drumMag.statBonusDurabilityMax = 80.0f;
    drumMag.weightImpact = 2.1f;
    m_recipes[drumMag.recipeId] = drumMag;

    // 1003: Тактический глушитель с теплорассеивателем
    ModRecipe suppressor;
    suppressor.recipeId = 1003;
    suppressor.category = ModCategory::Weapon;
    suppressor.slotType = ModSlot::Muzzle;
    suppressor.targetId = 101;
    suppressor.recipeName = "Интегрированный титановый супрессор";
    suppressor.requiredPerkLevel = 2;
    suppressor.requiredWorkbenchTier = 2;
    suppressor.craftTimeSeconds = 7.0f;
    suppressor.materialCosts = { {2002, 10}, {2006, 8} };
    suppressor.statBonusDamage = -2.5f; // Легкое падение начальной скорости пули
    suppressor.statBonusRange = -10.0f;
    suppressor.statBonusRecoil = -0.3f;
    suppressor.statBonusDurabilityMax = 60.0f;
    suppressor.weightImpact = 0.8f;
    m_recipes[suppressor.recipeId] = suppressor;

    // 1004: Снайперский прицел переменной кратности (3x-9x)
    ModRecipe sniperScope;
    sniperScope.recipeId = 1004;
    sniperScope.category = ModCategory::Weapon;
    sniperScope.slotType = ModSlot::Optics;
    sniperScope.targetId = 102; // Снайперская платформа
    sniperScope.recipeName = "Оптический прицел ПСО-ВТ (Кратность 3x-9x)";
    sniperScope.requiredPerkLevel = 3;
    sniperScope.requiredWorkbenchTier = 2;
    sniperScope.craftTimeSeconds = 10.0f;
    sniperScope.materialCosts = { {2002, 8}, {2004, 5}, {2003, 2} }; // Оптическое стекло, электроника
    sniperScope.statBonusDamage = 5.0f;
    sniperScope.statBonusRange = 140.0f;
    sniperScope.statBonusRecoil = 0.1f;
    sniperScope.statBonusDurabilityMax = 50.0f;
    sniperScope.weightImpact = 1.1f;
    m_recipes[sniperScope.recipeId] = sniperScope;

    // ------------------------------------------------------------------------
    // 2. СИЛОВАЯ БРОНЯ: T-60 / X-01 / РЕАКТОРЫ
    // ------------------------------------------------------------------------

    // 2001: Свинцовое экранирование торса (Защита от рад-излучения)
    ModRecipe leadShielding;
    leadShielding.recipeId = 2001;
    leadShielding.category = ModCategory::PowerArmor;
    leadShielding.slotType = ModSlot::ArmorPlating;
    leadShielding.targetId = 501; // Торс T-60
    leadShielding.recipeName = "Свинцовая многослойная футеровка пластин торса";
    leadShielding.requiredPerkLevel = 2;
    leadShielding.requiredWorkbenchTier = 2;
    leadShielding.craftTimeSeconds = 14.0f;
    leadShielding.materialCosts = { {2002, 22}, {2007, 18} }; // Свинец, Сталь
    leadShielding.statBonusDamage = 25.0f; // Защита от баллистики
    leadShielding.statBonusRange = 0.0f;
    leadShielding.statBonusRecoil = 0.0f;
    leadShielding.statBonusDurabilityMax = 300.0f;
    leadShielding.weightImpact = 18.0f;
    m_recipes[leadShielding.recipeId] = leadShielding;

    // 2002: Реактивный ранец (Jetpack) оператора
    ModRecipe paJetpack;
    paJetpack.recipeId = 2002;
    paJetpack.category = ModCategory::PowerArmor;
    paJetpack.slotType = ModSlot::UtilityBackpack;
    paJetpack.targetId = 501;
    paJetpack.recipeName = "Турбореактивный ранец вертикального подъема Mk.IV";
    paJetpack.requiredPerkLevel = 4;
    paJetpack.requiredWorkbenchTier = 3;
    paJetpack.craftTimeSeconds = 25.0f;
    paJetpack.materialCosts = { {2002, 40}, {2006, 25}, {2003, 12}, {2005, 10} };
    paJetpack.statBonusDamage = 0.0f;
    paJetpack.statBonusRange = 0.0f;
    paJetpack.statBonusRecoil = 0.0f;
    paJetpack.statBonusDurabilityMax = 250.0f;
    paJetpack.weightImpact = 24.0f;
    m_recipes[paJetpack.recipeId] = paJetpack;

    // 2003: Гидравлические сервоприводы Overdrive для ног
    ModRecipe overdriveServos;
    overdriveServos.recipeId = 2003;
    overdriveServos.category = ModCategory::PowerArmor;
    overdriveServos.slotType = ModSlot::Servomotors;
    overdriveServos.targetId = 504; // Ноги экзоскелета
    overdriveServos.recipeName = "Форсированные гидравлические сервоприводы 'Overdrive'";
    overdriveServos.requiredPerkLevel = 3;
    overdriveServos.requiredWorkbenchTier = 3;
    overdriveServos.craftTimeSeconds = 12.0f;
    overdriveServos.materialCosts = { {2002, 16}, {2003, 6}, {2006, 12} };
    overdriveServos.statBonusDamage = 15.0f;
    overdriveServos.statBonusRange = 0.0f;
    overdriveServos.statBonusRecoil = 0.0f;
    overdriveServos.statBonusDurabilityMax = 180.0f;
    overdriveServos.weightImpact = 8.5f;
    m_recipes[overdriveServos.recipeId] = overdriveServos;

    // ------------------------------------------------------------------------
    // 3. ТРАНСПОРТ И БРОНЕМАШИНЫ (VEHICLES)
    // ------------------------------------------------------------------------

    // 3001: Противокумулятивные экраны для тяжелого БТР
    ModRecipe vehicleCage;
    vehicleCage.recipeId = 3001;
    vehicleCage.category = ModCategory::Vehicle;
    vehicleCage.slotType = ModSlot::ArmorPlating;
    vehicleCage.targetId = 701;
    vehicleCage.recipeName = "Решетчатые противокумулятивные стальные экраны";
    vehicleCage.requiredPerkLevel = 2;
    vehicleCage.requiredWorkbenchTier = 2;
    vehicleCage.craftTimeSeconds = 18.0f;
    vehicleCage.materialCosts = { {2002, 65}, {2006, 20} };
    vehicleCage.statBonusDamage = 90.0f;
    vehicleCage.statBonusRange = 0.0f;
    vehicleCage.statBonusRecoil = 0.0f;
    vehicleCage.statBonusDurabilityMax = 800.0f;
    vehicleCage.weightImpact = 85.0f;
    m_recipes[vehicleCage.recipeId] = vehicleCage;

    // 3002: Усиленный турбокомпрессор дизельного двигателя
    ModRecipe turboEngine;
    turboEngine.recipeId = 3002;
    turboEngine.category = ModCategory::Vehicle;
    turboEngine.slotType = ModSlot::EngineMod;
    turboEngine.targetId = 701;
    turboEngine.recipeName = "Двухступенчатый турбонагнетатель с интеркулером";
    turboEngine.requiredPerkLevel = 3;
    turboEngine.requiredWorkbenchTier = 3;
    turboEngine.craftTimeSeconds = 20.0f;
    turboEngine.materialCosts = { {2002, 30}, {2005, 15}, {2003, 5} };
    turboEngine.statBonusDamage = 0.0f;
    turboEngine.statBonusRange = 0.0f;
    turboEngine.statBonusRecoil = 0.0f;
    turboEngine.statBonusDurabilityMax = 400.0f;
    turboEngine.weightImpact = 35.0f;
    m_recipes[turboEngine.recipeId] = turboEngine;

    // ------------------------------------------------------------------------
    // 4. ШАГАЮЩИЕ ТИТАНЫ И ПАУКООБРАЗНЫЕ ПЛАТФОРМЫ (TITANS)
    // ------------------------------------------------------------------------

    // 4001: Гироскопический демпфер балансировки для 4-х лап
    ModRecipe titanGyro;
    titanGyro.recipeId = 4001;
    titanGyro.category = ModCategory::TitanChassis;
    titanGyro.slotType = ModSlot::ChassisStabilizer;
    titanGyro.targetId = 801; // Четырехногий титан
    titanGyro.recipeName = "Тяжелый гиростабилизатор курса и динамического шага";
    titanGyro.requiredPerkLevel = 4;
    titanGyro.requiredWorkbenchTier = 4;
    titanGyro.craftTimeSeconds = 30.0f;
    titanGyro.materialCosts = { {2002, 80}, {2005, 30}, {2003, 20} };
    titanGyro.statBonusDamage = 50.0f;
    titanGyro.statBonusRange = 0.0f;
    titanGyro.statBonusRecoil = -0.45f;
    titanGyro.statBonusDurabilityMax = 1200.0f;
    titanGyro.weightImpact = 140.0f;
    m_recipes[titanGyro.recipeId] = titanGyro;

    // 4002: Катодные защитные рефлекторы щита
    ModRecipe titanShield;
    titanShield.recipeId = 4002;
    titanShield.category = ModCategory::TitanChassis;
    titanShield.slotType = ModSlot::EnergyBarrier;
    titanShield.targetId = 801;
    titanShield.recipeName = "Энергетический щитовой генератор 'Вортекс'";
    titanShield.requiredPerkLevel = 4;
    titanShield.requiredWorkbenchTier = 4;
    titanShield.craftTimeSeconds = 35.0f;
    titanShield.materialCosts = { {2003, 35}, {2005, 25}, {2006, 30} };
    titanShield.statBonusDamage = 120.0f;
    titanShield.statBonusRange = 0.0f;
    titanShield.statBonusRecoil = 0.0f;
    titanShield.statBonusDurabilityMax = 600.0f;
    titanShield.weightImpact = 75.0f;
    m_recipes[titanShield.recipeId] = titanShield;

    m_totalRecipesLoaded = static_cast<uint32_t>(m_recipes.size());
    Platform::Log("[MOD CATALOG]: Полная матрица чертежей инициализирована. Загружено рецептов: " + std::to_string(m_totalRecipesLoaded));
}

// ============================================================================
// SECTION 3: WORKBENCH STATUS, TIER PROGRESSION, HEAT DYNAMICS
// ============================================================================

void ModificationSystem::OpenWorkbench(ModCategory category) noexcept {
    if (!m_stationPowerActive) {
        Platform::Log("[WORKBENCH CRITICAL]: Верстак обесточен. Проверьте генератор базы!");
        return;
    }

    if (m_stationHeatLevel >= MAX_WORKBENCH_HEAT) {
        Platform::Log("[WORKBENCH CRITICAL]: Верстак перегрет! Требуется цикл охлаждения.");
        return;
    }

    m_workbenchActive = true;
    m_selectedCategory = category;
    Platform::Log("[WORKBENCH ONLINE]: Станция запущена. Фильтр категории: " + std::string(ModCategoryToString(category)));
}

void ModificationSystem::CloseWorkbench() noexcept {
    m_workbenchActive = false;
    Platform::Log("[WORKBENCH OFFLINE]: Сессия работы с верстаком завершена.");
}

bool ModificationSystem::IsWorkbenchActive() const noexcept {
    return m_workbenchActive && m_stationPowerActive && (m_stationHeatLevel < MAX_WORKBENCH_HEAT);
}

void ModificationSystem::SetStationPowerState(bool powerOn) noexcept {
    m_stationPowerActive = powerOn;
    if (!powerOn && m_workbenchActive) {
        m_workbenchActive = false;
        Platform::Log("[WORKBENCH WARNING]: Аварийное отключение питания верстака!");
    }
}

bool ModificationSystem::UpgradeWorkbenchTier(Player& player) {
    if (m_workbenchTierLevel >= 4) {
        Platform::Log("[WORKBENCH UPGRADE]: Достигнут предельный 4-й технологический уровень верстака.");
        return false;
    }

    // Затраты на апгрейд верстака по уровням
    std::unordered_map<uint32_t, uint16_t> upgradeCosts;
    switch (m_workbenchTierLevel) {
        case 1: // Апгрейд до Tier 2
            upgradeCosts = { {2002, 40}, {2006, 15} }; // Сталь, Алюминий
            break;
        case 2: // Апгрейд до Tier 3
            upgradeCosts = { {2002, 70}, {2005, 25}, {2003, 8} }; // Сталь, Сплав, Ядра
            break;
        case 3: // Апгрейд до Tier 4 (Военный уровень)
            upgradeCosts = { {2002, 120}, {2005, 50}, {2003, 20}, {2004, 15} };
            break;
    }

    if (!VerifyAndDeductMaterials(player, upgradeCosts)) {
        Platform::Log("[WORKBENCH UPGRADE FAIL]: Недостаточно ресурсов для модернизации станочной базы.");
        return false;
    }

    m_workbenchTierLevel++;
    m_craftingSpeedMultiplier += 0.25f; // Повышение скорости работы станка на каждом уровне

    Platform::Log("[WORKBENCH UPGRADED]: Верстак успешно модернизирован до уровня Tier " + std::to_string(m_workbenchTierLevel));
    return true;
}

void ModificationSystem::UpdateWorkbenchTick(float deltaTime) noexcept {
    // 1. Охлаждение рабочей поверхности станка
    if (m_stationHeatLevel > 0.0f) {
        m_stationHeatLevel -= HEAT_COOLDOWN_RATE * deltaTime;
        if (m_stationHeatLevel < 0.0f) {
            m_stationHeatLevel = 0.0f;
        }
    }

    // 2. Обработка очереди крафта (Crafting Queue)
    if (!m_craftingQueue.empty() && m_stationPowerActive) {
        auto& currentTask = m_craftingQueue.front();
        currentTask.progressSeconds += deltaTime * m_craftingSpeedMultiplier;

        // Генерация тепла станком при изготовлении деталей
        m_stationHeatLevel += 1.5f * deltaTime;
        if (m_stationHeatLevel > MAX_WORKBENCH_HEAT) {
            m_stationHeatLevel = MAX_WORKBENCH_HEAT;
            Platform::Log("[WORKBENCH THERMAL WARNING]: Станок вошел в зону термического троттлинга!");
        }

        if (currentTask.progressSeconds >= currentTask.totalDurationSeconds) {
            CompleteCraftTask(currentTask);
            m_craftingQueue.erase(m_craftingQueue.begin());
            m_totalCraftsCompleted++;
        }
    }
}

// ============================================================================
// SECTION 4: INVENTORY MATERIAL AUDITING & INVENTORY MANAGEMENT
// ============================================================================

bool ModificationSystem::VerifyAndDeductMaterials(Player& player, const std::unordered_map<uint32_t, uint16_t>& costs) {
    const auto& inventory = player.GetInventory();

    // Этап 1: Строгая верификация наличия требуемых материалов
    for (const auto& [matId, reqQty] : costs) {
        uint32_t currentCount = 0;
        for (const auto& item : inventory) {
            if (item.id == matId) {
                currentCount += item.quantity;
            }
        }

        if (currentCount < reqQty) {
            Platform::Log("[CRAFT AUDIT]: Нехватка ресурса ID " + std::to_string(matId) + 
                          ". Требуется: " + std::to_string(reqQty) + ", в наличии: " + std::to_string(currentCount));
            return false;
        }
    }

    // Этап 2: Поштучное безопасное списание материалов из слотов
    for (const auto& [matId, reqQty] : costs) {
        uint16_t pendingToTake = reqQty;

        for (size_t i = 0; i < inventory.size(); ++i) {
            if (inventory[i].id == matId) {
                uint16_t toDeduct = std::min(pendingToTake, inventory[i].quantity);
                player.RemoveItem(i, toDeduct);
                pendingToTake -= toDeduct;
                if (pendingToTake == 0) break;
            }
        }
    }

    Platform::Log("[CRAFT AUDIT]: Все материальные затраты успешно списаны со склада выжившего.");
    return true;
}

// ============================================================================
// SECTION 5: WEAPON UPGRADE PIPELINE
// ============================================================================

bool ModificationSystem::ApplyWeaponModification(Player& player, WeaponSystem& weaponSystem, uint32_t recipeId) {
    if (!IsWorkbenchActive()) {
        Platform::Log("[MOD ERROR]: Верстак не готов к работе (обесточен или перегрет).");
        return false;
    }

    auto it = m_recipes.find(recipeId);
    if (it == m_recipes.end() || it->second.category != ModCategory::Weapon) {
        Platform::Log("[MOD ERROR]: Оружейный чертеж ID " + std::to_string(recipeId) + " отсутствует в базе.");
        return false;
    }

    const ModRecipe& recipe = it->second;

    if (m_workbenchTierLevel < recipe.requiredWorkbenchTier) {
        Platform::Log("[MOD ERROR]: Недостаточный уровень станка. Требуется Tier " + std::to_string(recipe.requiredWorkbenchTier));
        return false;
    }

    if (player.GetLevel() < recipe.requiredPerkLevel) {
        Platform::Log("[MOD ERROR]: Недостаточный уровень навыка персонажа. Требуется уровень: " + std::to_string(recipe.requiredPerkLevel));
        return false;
    }

    if (!VerifyAndDeductMaterials(player, recipe.materialCosts)) {
        return false;
    }

    // Фиксация активной модификации
    m_activeModifications[recipe.targetId] = recipeId;
    m_stationHeatLevel += 12.0f; // Термический след от фрезерной обработки детали

    Platform::Log("[WEAPON WORKBENCH]: Модификация '" + recipe.recipeName + "' успешно смонтирована на оружие ID " + std::to_string(recipe.targetId));
    return true;
}

bool ModificationSystem::RemoveWeaponModification(Player& player, uint32_t targetWeaponId, uint32_t targetSlotIndex) {
    if (!IsWorkbenchActive()) return false;

    auto it = m_activeModifications.find(targetWeaponId);
    if (it == m_activeModifications.end()) {
        Platform::Log("[MOD ERROR]: На оружии ID " + std::to_string(targetWeaponId) + " нет установленных кастомных модулей.");
        return false;
    }

    uint32_t installedRecipeId = it->second;
    m_activeModifications.erase(it);

    // При демонтаже возвращаем 40% базовых материалов в виде металлолома
    auto recipeIt = m_recipes.find(installedRecipeId);
    if (recipeIt != m_recipes.end()) {
        player.AddItemToInventory(2002, 5); // Возврат 5 единиц стали
    }

    Platform::Log("[WEAPON WORKBENCH]: Модуль демонтирован с оружия ID " + std::to_string(targetWeaponId) + ". Часть ресурсов возвращена.");
    return true;
}

// ============================================================================
// SECTION 6: POWER ARMOR MODIFICATION PIPELINE
// ============================================================================

bool ModificationSystem::ApplyPowerArmorModification(Player& player, PowerArmorEngineContext& powerArmor, uint32_t recipeId) {
    if (!IsWorkbenchActive()) return false;

    auto it = m_recipes.find(recipeId);
    if (it == m_recipes.end() || it->second.category != ModCategory::PowerArmor) {
        Platform::Log("[MOD ERROR]: Чертеж силовой брони ID " + std::to_string(recipeId) + " не найден.");
        return false;
    }

    const ModRecipe& recipe = it->second;

    if (m_workbenchTierLevel < recipe.requiredWorkbenchTier) {
        Platform::Log("[MOD ERROR]: Для работ по силовой броне требуется станок Tier " + std::to_string(recipe.requiredWorkbenchTier));
        return false;
    }

    if (!VerifyAndDeductMaterials(player, recipe.materialCosts)) {
        return false;
    }

    // Ремонт и армирование сегмента силовой брони
    powerArmor.RepairComponent(ArmorComponentID::Torso, recipe.statBonusDurabilityMax);
    m_activeModifications[recipe.targetId] = recipeId;
    m_stationHeatLevel += 18.0f; // Тяжелая электросварка титановых листов

    Platform::Log("[EXOSKELETON WORKBENCH]: Бронеплиты экзоскелета усилены модулем: " + recipe.recipeName);
    return true;
}

// ============================================================================
// SECTION 7: VEHICLE & TITAN CHASSIS MODIFICATION PIPELINE
// ============================================================================

bool ModificationSystem::ApplyVehicleModification(Player& player, uint32_t vehicleId, uint32_t recipeId) {
    if (!IsWorkbenchActive()) return false;

    auto it = m_recipes.find(recipeId);
    if (it == m_recipes.end() || it->second.category != ModCategory::Vehicle) {
        Platform::Log("[MOD ERROR]: Чертеж шасси техники ID " + std::to_string(recipeId) + " некорректен.");
        return false;
    }

    const ModRecipe& recipe = it->second;

    if (m_workbenchTierLevel < recipe.requiredWorkbenchTier) {
        Platform::Log("[MOD ERROR]: Недостаточный уровень оборудования для модернизации тяжелой бронетехники.");
        return false;
    }

    if (!VerifyAndDeductMaterials(player, recipe.materialCosts)) {
        return false;
    }

    m_activeModifications[vehicleId] = recipeId;
    m_stationHeatLevel += 22.0f;

    Platform::Log("[CHASSIS WORKBENCH]: Шасси транспорта ID " + std::to_string(vehicleId) + " модернизировано: " + recipe.recipeName);
    return true;
}

bool ModificationSystem::ApplyTitanModification(Player& player, uint32_t titanId, uint32_t recipeId) {
    if (!IsWorkbenchActive()) return false;

    auto it = m_recipes.find(recipeId);
    if (it == m_recipes.end() || it->second.category != ModCategory::TitanChassis) {
        Platform::Log("[MOD ERROR]: Чертеж шасси титана ID " + std::to_string(recipeId) + " не найден.");
        return false;
    }

    const ModRecipe& recipe = it->second;

    if (m_workbenchTierLevel < 4) {
        Platform::Log("[MOD ERROR]: Сборка компонентов Титанов требует военного верстака Tier 4.");
        return false;
    }

    if (!VerifyAndDeductMaterials(player, recipe.materialCosts)) {
        return false;
    }

    m_activeModifications[titanId] = recipeId;
    m_stationHeatLevel += 35.0f; // Экстремальный нагрев станка при калибровке гиростабилизаторов

    Platform::Log("[TITAN ASSEMBLY]: Узел шагающего Титана ID " + std::to_string(titanId) + " откалиброван: " + recipe.recipeName);
    return true;
}

// ============================================================================
// SECTION 8: SALVAGE, SCRAPPING & DISASSEMBLY PIPELINE
// ============================================================================

bool ModificationSystem::SalvageItem(Player& player, uint32_t inventorySlotIndex) {
    if (!IsWorkbenchActive()) return false;

    const auto& inventory = player.GetInventory();
    if (inventorySlotIndex >= inventory.size()) return false;

    const auto& item = inventory[inventorySlotIndex];
    uint32_t itemId = item.id;
    uint16_t itemQty = item.quantity;

    // Базовый расчет выхода вторсырья (сталь, сплавы, микросхемы)
    uint16_t steelYield = 0;
    uint16_t woodYield = 0;
    uint16_t componentsYield = 0;

    if (itemId >= 100 && itemId < 200) { // Оружие
        steelYield = 8 * itemQty;
        woodYield = 3 * itemQty;
        componentsYield = 1 * itemQty;
    } else if (itemId >= 500 && itemId < 600) { // Элементы силовой брони
        steelYield = 25 * itemQty;
        componentsYield = 4 * itemQty;
    } else { // Прочие предметы
        steelYield = 2 * itemQty;
    }

    // Удаляем разобранный предмет из инвентаря
    player.RemoveItem(inventorySlotIndex, itemQty);

    // Добавляем полученный лом в инвентарь выжившего
    if (steelYield > 0) player.AddItemToInventory(2002, steelYield);
    if (woodYield > 0) player.AddItemToInventory(2001, woodYield);
    if (componentsYield > 0) player.AddItemToInventory(2005, componentsYield);

    m_stationHeatLevel += 5.0f;
    Platform::Log("[SALVAGE SYSTEM]: Предмет ID " + std::to_string(itemId) + " утилизирован. Выход сырья: Сталь (+" + 
                  std::to_string(steelYield) + "), Дерево (+" + std::to_string(woodYield) + ")");
    return true;
}

// ============================================================================
// SECTION 9: REPAIR PIPELINE (FIELD & WORKBENCH OVERHAUL)
// ============================================================================

bool ModificationSystem::RepairItem(Player& player, uint32_t targetItemId, float repairAmount, bool isWorkbenchOverhaul) {
    // Ремонт на верстаке более эффективен и не срезает максимальную прочность детали
    std::unordered_map<uint32_t, uint16_t> repairCosts;
    if (isWorkbenchOverhaul) {
        repairCosts = { {2002, 6} }; // 6 стали
    } else {
        repairCosts = { {2002, 10} }; // В полевых условиях расход материалов выше
    }

    if (!VerifyAndDeductMaterials(player, repairCosts)) {
        Platform::Log("[REPAIR FAIL]: Недостаточно сырья для проведения ремонтных работ.");
        return false;
    }

    Platform::Log("[REPAIR SUCCESS]: Предмет ID " + std::to_string(targetItemId) + 
                  " успешно восстановлен на " + std::to_string(repairAmount) + " HP (" + 
                  (isWorkbenchOverhaul ? "Капитальный ремонт на станке" : "Полевая сварка") + ")");
    return true;
}

// ============================================================================
// SECTION 10: ASYNCHRONOUS CRAFTING QUEUE MANAGEMENT
// ============================================================================

bool ModificationSystem::QueueCraftTask(Player& player, uint32_t recipeId) {
    if (!IsWorkbenchActive()) return false;

    auto it = m_recipes.find(recipeId);
    if (it == m_recipes.end()) return false;

    const ModRecipe& recipe = it->second;

    if (m_craftingQueue.size() >= 8) {
        Platform::Log("[QUEUE FULL]: Очередь станка переполнена (максимум 8 задач).");
        return false;
    }

    if (!VerifyAndDeductMaterials(player, recipe.materialCosts)) {
        return false;
    }

    CraftTask task;
    task.taskId = static_cast<uint32_t>(m_craftingQueue.size() + 1);
    task.recipeId = recipeId;
    task.totalDurationSeconds = recipe.craftTimeSeconds;
    task.progressSeconds = 0.0f;

    m_craftingQueue.push_back(task);
    Platform::Log("[CRAFT QUEUE]: Задача '" + recipe.recipeName + "' добавлена в очередь изготовления.");
    return true;
}

void ModificationSystem::CompleteCraftTask(const CraftTask& task) {
    auto it = m_recipes.find(task.recipeId);
    if (it != m_recipes.end()) {
        Platform::Log("[CRAFT COMPLETE]: Деталь '" + it->second.recipeName + "' успешно изготовлена на станке!");
    }
}

void ModificationSystem::CancelCraftTask(Player& player, size_t taskIndex) {
    if (taskIndex >= m_craftingQueue.size()) return;

    uint32_t recipeId = m_craftingQueue[taskIndex].recipeId;
    m_craftingQueue.erase(m_craftingQueue.begin() + taskIndex);

    // Возврат 75% материалов при отмене крафта
    auto it = m_recipes.find(recipeId);
    if (it != m_recipes.end()) {
        for (const auto& [matId, qty] : it->second.materialCosts) {
            uint16_t refund = static_cast<uint16_t>(qty * 0.75f);
            if (refund > 0) player.AddItemToInventory(matId, refund);
        }
    }

    Platform::Log("[CRAFT QUEUE]: Задача #" + std::to_string(taskIndex) + " отменена. Ресурсы возвращены.");
}

// ============================================================================
// SECTION 11: BINARY SERIALIZATION WITH INTEGRITY CHECKSUM (GHOST-RAM / DISK)
// ============================================================================

uint32_t ModificationSystem::CalculateChecksum(const std::vector<uint8_t>& buffer) noexcept {
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : buffer) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

std::vector<uint8_t> ModificationSystem::SerializeToBinary() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(1024);

    // Заголовок: Magic Number и Version
    const uint8_t* magicPtr = reinterpret_cast<const uint8_t*>(&MOD_SAVE_MAGIC);
    buffer.insert(buffer.end(), magicPtr, magicPtr + sizeof(uint32_t));

    const uint8_t* verPtr = reinterpret_cast<const uint8_t*>(&MOD_SAVE_VERSION);
    buffer.insert(buffer.end(), verPtr, verPtr + sizeof(uint32_t));

    // Параметры верстака
    buffer.push_back(m_stationPowerActive ? 1 : 0);
    buffer.push_back(static_cast<uint8_t>(m_workbenchTierLevel));

    const uint8_t* heatPtr = reinterpret_cast<const uint8_t*>(&m_stationHeatLevel);
    buffer.insert(buffer.end(), heatPtr, heatPtr + sizeof(float));

    // Упаковка установленных модификаций
    uint32_t modCount = static_cast<uint32_t>(m_activeModifications.size());
    const uint8_t* countPtr = reinterpret_cast<const uint8_t*>(&modCount);
    buffer.insert(buffer.end(), countPtr, countPtr + sizeof(uint32_t));

    for (const auto& [targetId, recipeId] : m_activeModifications) {
        const uint8_t* tPtr = reinterpret_cast<const uint8_t*>(&targetId);
        buffer.insert(buffer.end(), tPtr, tPtr + sizeof(uint32_t));

        const uint8_t* rPtr = reinterpret_cast<const uint8_t*>(&recipeId);
        buffer.insert(buffer.end(), rPtr, rPtr + sizeof(uint32_t));
    }

    // Вычисление и допись контрольной суммы
    uint32_t checksum = CalculateChecksum(buffer);
    const uint8_t* chkPtr = reinterpret_cast<const uint8_t*>(&checksum);
    buffer.insert(buffer.end(), chkPtr, chkPtr + sizeof(uint32_t));

    Platform::Log("[MOD SERIALIZE]: Контекст системы модификаций успешно упакован в бинарный поток (" + 
                  std::to_string(buffer.size()) + " байт).");
    return buffer;
}

bool ModificationSystem::DeserializeFromBinary(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < (sizeof(uint32_t) * 3 + 2 + sizeof(float))) {
        Platform::Log("[MOD DESERIALIZE ERROR]: Размер бинарного потока слишком мал.");
        return false;
    }

    // Проверка контрольной суммы
    size_t payloadSize = buffer.size() - sizeof(uint32_t);
    std::vector<uint8_t> payloadData(buffer.begin(), buffer.begin() + payloadSize);
    uint32_t expectedChecksum = CalculateChecksum(payloadData);

    uint32_t storedChecksum = 0;
    std::memcpy(&storedChecksum, buffer.data() + payloadSize, sizeof(uint32_t));

    if (expectedChecksum != storedChecksum) {
        Platform::Log("[MOD DESERIALIZE ERROR]: Нарушена контрольная сумма дампа модификаций!");
        return false;
    }

    size_t cursor = 0;

    uint32_t magic = 0;
    std::memcpy(&magic, buffer.data() + cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    if (magic != MOD_SAVE_MAGIC) {
        Platform::Log("[MOD DESERIALIZE ERROR]: Неверный Magic Identifier дампа.");
        return false;
    }

    uint32_t version = 0;
    std::memcpy(&version, buffer.data() + cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    m_stationPowerActive = (buffer[cursor++] != 0);
    m_workbenchTierLevel = static_cast<uint32_t>(buffer[cursor++]);

    std::memcpy(&m_stationHeatLevel, buffer.data() + cursor, sizeof(float));
    cursor += sizeof(float);

    uint32_t modCount = 0;
    std::memcpy(&modCount, buffer.data() + cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    m_activeModifications.clear();
    for (uint32_t i = 0; i < modCount; ++i) {
        if (cursor + sizeof(uint32_t) * 2 > payloadSize) return false;

        uint32_t targetId = 0;
        uint32_t recipeId = 0;

        std::memcpy(&targetId, buffer.data() + cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);

        std::memcpy(&recipeId, buffer.data() + cursor, sizeof(uint32_t));
        cursor += sizeof(uint32_t);

        m_activeModifications[targetId] = recipeId;
    }

    Platform::Log("[MOD DESERIALIZE]: Данные модификаций успешно восстановлены (" + 
                  std::to_string(m_activeModifications.size()) + " активных узлов).");
    return true;
}

// ============================================================================
// SECTION 12: TELEMETRY & METRIC STATUS QUERIES
// ============================================================================

uint32_t ModificationSystem::GetTotalCompletedCrafts() const noexcept {
    return m_totalCraftsCompleted;
}

float ModificationSystem::GetStationHeatLevel() const noexcept {
    return m_stationHeatLevel;
}

uint32_t ModificationSystem::GetWorkbenchTier() const noexcept {
    return m_workbenchTierLevel;
}

size_t ModificationSystem::GetQueuedTasksCount() const noexcept {
    return m_craftingQueue.size();
}

} // namespace Centralia