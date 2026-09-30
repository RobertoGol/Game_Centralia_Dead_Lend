#include "gameplay/ItemDatabase.hpp"
#include "platform/Platform.hpp"
#include "core/MemoryManager.hpp"
#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>
#include <stdexcept>
#include <cstring>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS, ENUMS & UNIFIED DATA STRUCTURES
// ============================================================================

enum class ItemType {
    Weapon,
    Armor,
    Consumable,
    Ammunition,
    Material,
    Junk,
    QuestItem
};

enum class ArmorSlot {
    None, Head, Chest, LeftArm, RightArm, LeftLeg, RightLeg, FullBody
};

enum class WeaponType {
    None, Unarmed, Melee, Pistol, Rifle, Shotgun, Heavy, Energy, Explosive
};

// Вложенные структуры для специфичных данных предмета
struct WeaponStats {
    WeaponType wType;
    float damagePhys;
    float damageEnergy;
    float damageRad;
    float fireRate;         // Выстрелов в секунду
    float range;            // Эффективная дальность (метры)
    float reloadTime;       // Секунды
    uint32_t ammoItemId;    // ID требуемых патронов
    uint32_t magazineSize;
    float critMultiplier;   // Множитель крита (например, 2.0x)
    uint32_t apCost;        // Стоимость выстрела в V.A.T.S.
};

struct ArmorStats {
    ArmorSlot slot;
    float dr; // Damage Resistance (Физ. броня)
    float er; // Energy Resistance (Энерг. броня)
    float rr; // Radiation Resistance (Рад. защита)
    float movementPenalty; // Штраф к скорости (0.0 - 1.0)
};

struct ConsumableStats {
    float hpRestore;
    float apRestore;
    float radRestore;     // Если отрицательное - лечит радиацию, если положительное - заражает
    float duration;       // Время действия баффа (0 = мгновенно)
    uint32_t addictionId; // ID болезни/зависимости
    float addictionChance;// Вероятность получить зависимость (0.0 - 100.0)
};

// Унифицированная структура предмета (Data-Oriented, без виртуальных таблиц)
struct ItemRecord {
    uint32_t id;
    std::string name;
    std::string description;
    ItemType type;
    
    float weight;         // Кг
    uint32_t baseValue;   // Базовая цена в крышках/кредитах
    uint32_t maxStack;    // Макс. количество в одном слоте
    float maxDurability;  // Максимальная прочность (0 = не ломается)
    
    // Union-подобная архитектура. Для простоты C++ и избежания std::variant, храним все структуры.
    WeaponStats weapon;
    ArmorStats armor;
    ConsumableStats consumable;
};

// Структуры для таблиц лута (Loot Tables)
struct LootEntry {
    uint32_t itemId;
    uint32_t weight;      // Вес (шанс выпадения)
    uint32_t minCount;
    uint32_t maxCount;
    float dropChance;     // Абсолютный шанс (0.0 - 100.0)
};

struct LootPool {
    std::string poolName;
    std::vector<LootEntry> entries;
    uint32_t totalWeight;
    uint32_t minItemsToRoll;
    uint32_t maxItemsToRoll;
};

// ============================================================================
// SECTION 2: SINGLETON & INTERNAL DATABASE STATE
// ============================================================================

struct ItemDatabaseImpl {
    std::unordered_map<uint32_t, ItemRecord> registry;
    std::unordered_map<std::string, LootPool> lootPools;
    std::mt19937 rngEngine;

    ItemDatabaseImpl() {
        // Инициализация генератора случайных чисел
        std::random_device rd;
        rngEngine.seed(rd());
    }

    void RegisterItem(const ItemRecord& record) {
        if (registry.find(record.id) != registry.end()) {
            Platform::Log("[DB ERROR]: Конфликт ID предметов! Предмет " + std::to_string(record.id) + " уже существует.");
            return;
        }
        registry[record.id] = record;
    }
};

ItemDatabase* ItemDatabase::s_instance = nullptr;

ItemDatabase::ItemDatabase() : m_pImpl(new ItemDatabaseImpl()) {
    if (s_instance) {
        Platform::Log("[DB FATAL]: Попытка двойной инициализации ItemDatabase!");
        std::terminate();
    }
    s_instance = this;

    Platform::Log("[ITEM DATABASE]: Инициализация базы данных предметов и таблиц лута...");
    PopulateHardcodedDatabase();
    PopulateLootPools();
    
    Platform::Log("[ITEM DATABASE]: Загружено предметов: " + std::to_string(m_pImpl->registry.size()) + 
                  " | Таблиц лута: " + std::to_string(m_pImpl->lootPools.size()));
}

ItemDatabase::~ItemDatabase() {
    m_pImpl->registry.clear();
    m_pImpl->lootPools.clear();
    delete m_pImpl;
    s_instance = nullptr;
    Platform::Log("[ITEM DATABASE]: База данных выгружена.");
}

ItemDatabase& ItemDatabase::GetInstance() {
    if (!s_instance) std::terminate();
    return *s_instance;
}

// ============================================================================
// SECTION 3: MASSIVE HARDCODED DATABASE POPULATION (FALLBACK / CORE DATA)
// ============================================================================

void ItemDatabase::PopulateHardcodedDatabase() {
    
    // --- 1. MATERIALS & JUNK (Материалы и Хлам для крафта) ---
    
    ItemRecord steel = {2002, "Сталь", "Прочный металлический сплав. Основа выживания.", ItemType::Material, 0.1f, 2, 500, 0.0f};
    m_pImpl->RegisterItem(steel);
    
    ItemRecord wood = {2001, "Древесина", "Сухие доски и ветки. Идут на растопку и базовое строительство.", ItemType::Material, 0.2f, 1, 500, 0.0f};
    m_pImpl->RegisterItem(wood);
    
    ItemRecord ductTape = {2050, "Изолента", "Чудо инженерной мысли. Чинит всё.", ItemType::Junk, 0.1f, 15, 100, 0.0f};
    m_pImpl->RegisterItem(ductTape);
    
    ItemRecord deskFan = {2051, "Настольный вентилятор", "Старый вентилятор. Можно разобрать на винты и шестеренки.", ItemType::Junk, 1.5f, 25, 10, 0.0f};
    m_pImpl->RegisterItem(deskFan);

    // --- 2. AMMUNITION (Патроны) ---

    ItemRecord ammo9mm = {600, "Патрон 9мм", "Стандартный пистолетный патрон.", ItemType::Ammunition, 0.01f, 1, 1000, 0.0f};
    m_pImpl->RegisterItem(ammo9mm);

    ItemRecord ammo556 = {601, "Патрон 5.56мм", "Промежуточный патрон для штурмовых винтовок.", ItemType::Ammunition, 0.015f, 2, 1000, 0.0f};
    m_pImpl->RegisterItem(ammo556);

    ItemRecord ammo12ga = {602, "Патрон 12 калибра (Дробь)", "Идеален для ближнего боя против мутантов.", ItemType::Ammunition, 0.03f, 3, 500, 0.0f};
    m_pImpl->RegisterItem(ammo12ga);

    ItemRecord ammoMicrofusion = {603, "Ядерная батарея", "Универсальный источник питания для энергетического оружия.", ItemType::Ammunition, 0.1f, 10, 200, 0.0f};
    m_pImpl->RegisterItem(ammoMicrofusion);

    // --- 3. CONSUMABLES (Медикаменты, Еда, Химия) ---

    ItemRecord stimpak;
    stimpak.id = 4002;
    stimpak.name = "Стимпак";
    stimpak.description = "Внутривенный инъектор. Мгновенно заживляет тяжелые раны.";
    stimpak.type = ItemType::Consumable;
    stimpak.weight = 0.1f; stimpak.baseValue = 50; stimpak.maxStack = 100; stimpak.maxDurability = 0.0f;
    stimpak.consumable = {30.0f, 0.0f, 0.0f, 0.0f, 0, 0.0f}; // Лечит 30 ХП мгновенно
    m_pImpl->RegisterItem(stimpak);

    ItemRecord radAway;
    radAway.id = 4003;
    radAway.name = "Антирадин";
    radAway.description = "Химический коктейль, выводящий изотопы из организма.";
    radAway.type = ItemType::Consumable;
    radAway.weight = 0.1f; radAway.baseValue = 80; radAway.maxStack = 100; radAway.maxDurability = 0.0f;
    radAway.consumable = {0.0f, 0.0f, -50.0f, 0.0f, 0, 0.0f}; // Снимает 50 Радиации
    m_pImpl->RegisterItem(radAway);

    ItemRecord psycho;
    psycho.id = 4008;
    psycho.name = "Психо";
    psycho.description = "Боевой стимулятор. Повышает урон и сопротивление, но вызывает привыкание.";
    psycho.type = ItemType::Consumable;
    psycho.weight = 0.1f; psycho.baseValue = 100; psycho.maxStack = 50; psycho.maxDurability = 0.0f;
    psycho.consumable = {0.0f, 25.0f, 0.0f, 60.0f, 101, 15.0f}; // Бафф на 60 сек, 15% шанс зависимости (ID 101)
    m_pImpl->RegisterItem(psycho);

    ItemRecord dirtyWater;
    dirtyWater.id = 3004;
    dirtyWater.name = "Грязная вода";
    dirtyWater.description = "Собрана из радиоактивной лужи. Пить только в крайнем случае.";
    dirtyWater.type = ItemType::Consumable;
    dirtyWater.weight = 0.5f; dirtyWater.baseValue = 5; dirtyWater.maxStack = 20; dirtyWater.maxDurability = 0.0f;
    dirtyWater.consumable = {10.0f, 0.0f, 10.0f, 0.0f, 0, 0.0f}; // +10 ХП, но +10 Радиации
    m_pImpl->RegisterItem(dirtyWater);

    // --- 4. WEAPONS (Оружие) ---

    ItemRecord pipePistol;
    pipePistol.id = 1001;
    pipePistol.name = "Самодельный пистолет";
    pipePistol.description = "Собран из водопроводных труб и изоленты. Ненадежен.";
    pipePistol.type = ItemType::Weapon;
    pipePistol.weight = 1.2f; pipePistol.baseValue = 25; pipePistol.maxStack = 1; pipePistol.maxDurability = 100.0f;
    pipePistol.weapon = {WeaponType::Pistol, 12.0f, 0.0f, 0.0f, 2.5f, 20.0f, 2.0f, 600, 6, 2.0f, 15};
    m_pImpl->RegisterItem(pipePistol);

    ItemRecord mm10Pistol;
    mm10Pistol.id = 1002;
    mm10Pistol.name = "10мм Пистолет N99";
    mm10Pistol.description = "Стандартное армейское оружие. Надежное и точное.";
    mm10Pistol.type = ItemType::Weapon;
    mm10Pistol.weight = 1.8f; mm10Pistol.baseValue = 150; mm10Pistol.maxStack = 1; mm10Pistol.maxDurability = 500.0f;
    mm10Pistol.weapon = {WeaponType::Pistol, 18.0f, 0.0f, 0.0f, 3.5f, 35.0f, 1.8f, 600, 12, 2.0f, 20};
    m_pImpl->RegisterItem(mm10Pistol);

    ItemRecord assaultRifle;
    assaultRifle.id = 1010;
    assaultRifle.name = "Штурмовая винтовка R91";
    assaultRifle.description = "Довоенная винтовка. Высокая скорострельность и урон.";
    assaultRifle.type = ItemType::Weapon;
    assaultRifle.weight = 4.5f; assaultRifle.baseValue = 450; assaultRifle.maxStack = 1; assaultRifle.maxDurability = 800.0f;
    assaultRifle.weapon = {WeaponType::Rifle, 24.0f, 0.0f, 0.0f, 8.0f, 75.0f, 2.5f, 601, 24, 2.0f, 35};
    m_pImpl->RegisterItem(assaultRifle);

    ItemRecord combatShotgun;
    combatShotgun.id = 1015;
    combatShotgun.name = "Боевой дробовик";
    combatShotgun.description = "Разрывает гулей в клочья в закрытых помещениях.";
    combatShotgun.type = ItemType::Weapon;
    combatShotgun.weight = 5.2f; combatShotgun.baseValue = 350; combatShotgun.maxStack = 1; combatShotgun.maxDurability = 600.0f;
    combatShotgun.weapon = {WeaponType::Shotgun, 65.0f, 0.0f, 0.0f, 1.5f, 15.0f, 3.0f, 602, 8, 1.5f, 40};
    m_pImpl->RegisterItem(combatShotgun);

    ItemRecord laserRifle;
    laserRifle.id = 1020;
    laserRifle.name = "Лазерная винтовка AER9";
    laserRifle.description = "Сфокусированный лазерный луч. Практически нет отдачи.";
    laserRifle.type = ItemType::Weapon;
    laserRifle.weight = 3.8f; laserRifle.baseValue = 600; laserRifle.maxStack = 1; laserRifle.maxDurability = 450.0f;
    laserRifle.weapon = {WeaponType::Energy, 0.0f, 30.0f, 0.0f, 4.0f, 120.0f, 2.0f, 603, 30, 2.5f, 30};
    m_pImpl->RegisterItem(laserRifle);

    ItemRecord minigun;
    minigun.id = 1030;
    minigun.name = "Миниган CZ53";
    minigun.description = "Шестиствольный пулемет. Сжирает патроны с ужасающей скоростью.";
    minigun.type = ItemType::Weapon;
    minigun.weight = 18.0f; minigun.baseValue = 1200; minigun.maxStack = 1; minigun.maxDurability = 1000.0f;
    minigun.weapon = {WeaponType::Heavy, 8.0f, 0.0f, 0.0f, 30.0f, 50.0f, 5.0f, 601, 150, 1.5f, 75}; // Очень быстрый огонь
    m_pImpl->RegisterItem(minigun);

    ItemRecord superSledge;
    superSledge.id = 1040;
    superSledge.name = "Суперкувалда";
    superSledge.description = "Кувалда с кинетическим усилителем. Ломает кости сквозь броню.";
    superSledge.type = ItemType::Weapon;
    superSledge.weight = 12.0f; superSledge.baseValue = 400; superSledge.maxStack = 1; superSledge.maxDurability = 2000.0f;
    superSledge.weapon = {WeaponType::Melee, 85.0f, 15.0f, 0.0f, 0.8f, 2.0f, 0.0f, 0, 0, 3.0f, 45};
    m_pImpl->RegisterItem(superSledge);

    // --- 5. ARMOR (Броня и Экипировка) ---

    ItemRecord leatherChest;
    leatherChest.id = 5001;
    leatherChest.name = "Кожаная броня (Нагрудник)";
    leatherChest.description = "Сшита из шкур браминов. Защищает от укусов и царапин.";
    leatherChest.type = ItemType::Armor;
    leatherChest.weight = 4.0f; leatherChest.baseValue = 75; leatherChest.maxStack = 1; leatherChest.maxDurability = 300.0f;
    leatherChest.armor = {ArmorSlot::Chest, 10.0f, 15.0f, 0.0f, 0.0f}; // Высокая защита от энергии (лазеры)
    m_pImpl->RegisterItem(leatherChest);

    ItemRecord combatChest;
    combatChest.id = 5010;
    combatChest.name = "Боевая броня (Нагрудник)";
    combatChest.description = "Тяжелые керамические пластины на кевларовой подкладке.";
    combatChest.type = ItemType::Armor;
    combatChest.weight = 12.0f; combatChest.baseValue = 450; combatChest.maxStack = 1; combatChest.maxDurability = 800.0f;
    combatChest.armor = {ArmorSlot::Chest, 35.0f, 25.0f, 10.0f, 0.1f}; // Небольшой штраф к скорости
    m_pImpl->RegisterItem(combatChest);

    ItemRecord powerArmorT51;
    powerArmorT51.id = 5100;
    powerArmorT51.name = "Силовая броня T-51 (Торс)";
    powerArmorT51.description = "Вершина довоенной инженерной мысли. Танк для пехотинца.";
    powerArmorT51.type = ItemType::Armor;
    powerArmorT51.weight = 45.0f; powerArmorT51.baseValue = 2500; powerArmorT51.maxStack = 1; powerArmorT51.maxDurability = 5000.0f;
    powerArmorT51.armor = {ArmorSlot::Chest, 150.0f, 120.0f, 150.0f, 0.3f}; // Огромная защита, но тяжело ходить
    m_pImpl->RegisterItem(powerArmorT51);

    ItemRecord gasMask;
    gasMask.id = 5050;
    gasMask.name = "Противогаз с фильтром";
    gasMask.description = "Спасает от радиоактивной пыли, но ограничивает обзор.";
    gasMask.type = ItemType::Armor;
    gasMask.weight = 1.0f; gasMask.baseValue = 120; gasMask.maxStack = 1; gasMask.maxDurability = 150.0f;
    gasMask.armor = {ArmorSlot::Head, 2.0f, 2.0f, 35.0f, 0.0f}; // Высокая RR
    m_pImpl->RegisterItem(gasMask);
}

// ============================================================================
// SECTION 4: LOOT TABLES & POOL INITIALIZATION
// ============================================================================

void ItemDatabase::PopulateLootPools() {
    auto addPool = [this](const std::string& name, const std::vector<LootEntry>& entries, uint32_t min, uint32_t max) {
        LootPool pool;
        pool.poolName = name;
        pool.entries = entries;
        pool.minItemsToRoll = min;
        pool.maxItemsToRoll = max;
        pool.totalWeight = 0;
        for (const auto& entry : entries) {
            pool.totalWeight += entry.weight;
        }
        m_pImpl->lootPools[name] = pool;
    };

    // Пул 1: Мусор и компоненты (Scrap)
    addPool("Pool_DeskScrap", {
        {2050, 50, 1, 2, 100.0f}, // Изолента
        {2051, 30, 1, 1, 100.0f}, // Вентилятор
        {2001, 100, 2, 5, 100.0f} // Дерево
    }, 1, 3);

    // Пул 2: Базовая Аптечка (First Aid)
    addPool("Pool_FirstAid", {
        {4002, 20, 1, 2, 60.0f},  // Стимпак (редкий)
        {4003, 30, 1, 1, 40.0f},  // Антирадин
        {4008, 10, 1, 1, 20.0f},  // Психо
        {3004, 80, 1, 3, 100.0f}  // Грязная вода
    }, 1, 2);

    // Пул 3: Оружейный ящик низкого уровня (Raider Cache)
    addPool("Pool_RaiderCache", {
        {1001, 100, 1, 1, 100.0f}, // Самопал
        {1002, 20,  1, 1, 100.0f}, // 10мм Пистолет
        {600,  80, 12, 36, 100.0f}, // Патроны 9мм
        {5001, 40,  1, 1, 50.0f}   // Кожаная броня
    }, 2, 4);

    // Пул 4: Сейф Босса (High-Tier Military)
    addPool("Pool_MilitarySafe", {
        {1010, 50, 1, 1, 100.0f}, // Штурмовая винтовка
        {1020, 30, 1, 1, 100.0f}, // Лазерная винтовка
        {1030, 5,  1, 1, 100.0f}, // Миниган (Крайне редкий)
        {5010, 40, 1, 1, 100.0f}, // Боевая броня
        {601, 100, 30, 90, 100.0f}, // Патроны 5.56
        {603, 80,  10, 25, 100.0f}  // Ядерные батареи
    }, 3, 5);
}

// ============================================================================
// SECTION 5: QUERY APIs (RETRIEVING ITEM DATA)
// ============================================================================

bool ItemDatabase::HasItem(uint32_t itemId) const noexcept {
    return m_pImpl->registry.find(itemId) != m_pImpl->registry.end();
}

const ItemRecord* ItemDatabase::GetItemRecord(uint32_t itemId) const {
    auto it = m_pImpl->registry.find(itemId);
    if (it != m_pImpl->registry.end()) {
        return &(it->second);
    }
    Platform::Log("[DB WARNING]: Запрошен несуществующий предмет ID: " + std::to_string(itemId));
    return nullptr;
}

std::string ItemDatabase::GetItemName(uint32_t itemId) const {
    const ItemRecord* record = GetItemRecord(itemId);
    return record ? record->name : "Unknown Item";
}

float ItemDatabase::GetItemWeight(uint32_t itemId) const {
    const ItemRecord* record = GetItemRecord(itemId);
    return record ? record->weight : 0.0f;
}

// ============================================================================
// SECTION 6: RNG INSTANCE GENERATION (CONDITION, DURABILITY, VALUE)
// ============================================================================

ItemInstance ItemDatabase::GenerateItemInstance(uint32_t itemId) const {
    ItemInstance instance;
    instance.itemId = itemId;
    instance.quantity = 1;
    instance.currentDurability = -1.0f;
    instance.marketValueOverride = -1;

    const ItemRecord* record = GetItemRecord(itemId);
    if (!record) return instance;

    // Если предмет ломаемый (Оружие/Броня), генерируем случайное состояние
    if (record->maxDurability > 0.0f) {
        std::uniform_real_distribution<float> conditionDist(0.15f, 0.95f); // Состояние от 15% до 95%
        float conditionMultiplier = conditionDist(m_pImpl->rngEngine);
        
        instance.currentDurability = record->maxDurability * conditionMultiplier;

        // Расчет рыночной цены в зависимости от состояния (Сломанное стоит копейки)
        // Формула: BaseValue * (Condition^1.5)
        float valueMult = std::pow(conditionMultiplier, 1.5f);
        instance.marketValueOverride = static_cast<uint32_t>(record->baseValue * valueMult);
        
        // Убитое оружие в 10 раз дешевле, чем новое
        if (instance.marketValueOverride < 1) instance.marketValueOverride = 1;
    } else {
        instance.currentDurability = record->maxDurability;
        instance.marketValueOverride = record->baseValue;
    }

    return instance;
}

std::vector<ItemInstance> ItemDatabase::GenerateLootFromPool(const std::string& poolName) const {
    std::vector<ItemInstance> loot;
    
    auto it = m_pImpl->lootPools.find(poolName);
    if (it == m_pImpl->lootPools.end()) {
        Platform::Log("[LOOT ERROR]: Пул лута '" + poolName + "' не найден!");
        return loot;
    }

    const LootPool& pool = it->second;
    
    std::uniform_int_distribution<uint32_t> rollCountDist(pool.minItemsToRoll, pool.maxItemsToRoll);
    uint32_t rolls = rollCountDist(m_pImpl->rngEngine);
    
    std::uniform_real_distribution<float> chanceDist(0.0f, 100.0f);

    for (uint32_t i = 0; i < rolls; ++i) {
        // Взвешенный рандом (Weighted RNG)
        std::uniform_int_distribution<uint32_t> weightDist(0, pool.totalWeight);
        uint32_t rollWeight = weightDist(m_pImpl->rngEngine);
        
        uint32_t currentWeight = 0;
        for (const auto& entry : pool.entries) {
            currentWeight += entry.weight;
            if (rollWeight <= currentWeight) {
                // Проверяем абсолютный шанс дропа
                if (chanceDist(m_pImpl->rngEngine) <= entry.dropChance) {
                    
                    std::uniform_int_distribution<uint32_t> qtyDist(entry.minCount, entry.maxCount);
                    uint32_t qty = qtyDist(m_pImpl->rngEngine);
                    
                    ItemInstance item = GenerateItemInstance(entry.itemId);
                    item.quantity = qty;
                    loot.push_back(item);
                }
                break; // Выбрали предмет из весов, переходим к следующему роллу
            }
        }
    }

    // Склеиваем одинаковые предметы (Stacking)
    std::vector<ItemInstance> stackedLoot;
    for (const auto& item : loot) {
        bool stacked = false;
        const ItemRecord* rec = GetItemRecord(item.itemId);
        
        // Стакаем только предметы без прочности (патроны, материалы, стимпаки)
        if (rec && rec->maxDurability == 0.0f) {
            for (auto& sItem : stackedLoot) {
                if (sItem.itemId == item.itemId) {
                    sItem.quantity += item.quantity;
                    stacked = true;
                    break;
                }
            }
        }
        
        if (!stacked) {
            stackedLoot.push_back(item);
        }
    }

    return stackedLoot;
}

// ============================================================================
// SECTION 7: COMBAT MATH & WEAPON BALLISTICS
// ============================================================================

float ItemDatabase::CalculateWeaponDamage(uint32_t weaponId, float currentDurability, float playerSkillLevel) const {
    const ItemRecord* record = GetItemRecord(weaponId);
    if (!record || record->type != ItemType::Weapon) return 0.0f;

    float baseDamage = record->weapon.damagePhys + record->weapon.damageEnergy;

    // 1. Деградация урона от износа оружия
    // Если прочность ниже 50%, урон начинает падать. На 1% прочности урон равен 50% от номинала.
    float conditionPercentage = currentDurability / record->maxDurability;
    float conditionMultiplier = 1.0f;
    if (conditionPercentage < 0.5f) {
        conditionMultiplier = 0.5f + conditionPercentage; // Интерполяция от 0.5 до 1.0
    }

    // 2. Скейлинг от навыков игрока (Skill Level от 0 до 100)
    // 100 навыка дает +50% к урону
    float skillMultiplier = 1.0f + (playerSkillLevel / 100.0f) * 0.5f;

    float finalDamage = baseDamage * conditionMultiplier * skillMultiplier;
    
    // В реальной игре здесь еще плюсуется урон от надетых модификаций (Mod System)
    
    return finalDamage;
}

float ItemDatabase::CalculateArmorReduction(uint32_t armorId, float incomingPhysical, float incomingEnergy) const {
    const ItemRecord* record = GetItemRecord(armorId);
    if (!record || record->type != ItemType::Armor) return incomingPhysical + incomingEnergy;

    // Алгоритм бронепробития (Fallout 4 Formula Variation)
    // Коэффициент защиты зависит от отношения Урона к Броне
    
    auto calculateMitigatedDamage = [](float damage, float resistance) -> float {
        if (damage <= 0.0f) return 0.0f;
        if (resistance <= 0.0f) return damage;
        
        // Damage = Damage * min(0.99, (Damage / (Resistance * 0.15))^0.365)
        float ratio = damage / (resistance * 0.15f);
        float multiplier = std::pow(ratio, 0.365f);
        multiplier = std::min(0.99f, multiplier);
        
        return damage * multiplier;
    };

    float mitigatedPhys = calculateMitigatedDamage(incomingPhysical, record->armor.dr);
    float mitigatedEnergy = calculateMitigatedDamage(incomingEnergy, record->armor.er);

    return mitigatedPhys + mitigatedEnergy;
}

} // namespace Centralia