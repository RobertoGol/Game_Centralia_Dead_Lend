#include "gameplay/ClassSystem.hpp"
#include "platform/Platform.hpp"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <fstream>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS, LOOKUP TABLES & EXPERIENCE CURVES
// ============================================================================

namespace {
    constexpr uint32_t CLASS_SYSTEM_MAGIC = 0x52504753; // "RPGS"
    constexpr uint32_t CLASS_SYSTEM_VERSION = 2;
    constexpr uint32_t MAX_CHARACTER_LEVEL = 100;
    constexpr uint32_t MAX_ATTRIBUTE_VALUE = 20;
    constexpr uint32_t MAX_SKILL_VALUE = 100;

    const char* AttributeToString(CharacterAttribute attr) noexcept {
        switch (attr) {
            case CharacterAttribute::Strength:     return "Сила (STR)";
            case CharacterAttribute::Perception:   return "Восприятие (PER)";
            case CharacterAttribute::Endurance:    return "Выносливость (END)";
            case CharacterAttribute::Charisma:     return "Харизма (CHR)";
            case CharacterAttribute::Intelligence: return "Интеллект (INT)";
            case CharacterAttribute::Agility:      return "Ловкость (AGI)";
            case CharacterAttribute::Luck:         return "Удача (LCK)";
            default:                               return "Неизвестный атрибут";
        }
    }

    const char* SkillToString(CharacterSkill skill) noexcept {
        switch (skill) {
            case CharacterSkill::Firearms:   return "Огнестрельное оружие";
            case CharacterSkill::Melee:      return "Ближний бой";
            case CharacterSkill::Survival:   return "Выживание";
            case CharacterSkill::Medicine:   return "Медицина";
            case CharacterSkill::Mechanics:  return "Механика и Крафт";
            case CharacterSkill::Stealth:    return "Скрытность";
            case CharacterSkill::Speech:     return "Красноречие";
            default:                         return "Неизвестный навык";
        }
    }

    const char* ArchetypeToString(ClassArchetype archetype) noexcept {
        switch (archetype) {
            case ClassArchetype::Mercenary:   return "Наемник (Mercenary)";
            case ClassArchetype::Scavenger:   return "Мусорщик (Scavenger)";
            case ClassArchetype::TechPriest:  return "Техножрец (Tech-Priest)";
            case ClassArchetype::Wastelander: return "Бродяга Пустошей (Wastelander)";
            case ClassArchetype::Raider:      return "Рейдер-Изгой (Raider)";
            default:                          return "Без класса";
        }
    }

    // Экспоненциальная кривая опыта: XP(lvl) = Base * (Lvl^1.5)
    uint32_t CalculateRequiredExperienceForLevel(uint32_t level) {
        if (level <= 1) return 0;
        return static_cast<uint32_t>(1000.0f * std::pow(static_cast<float>(level - 1), 1.5f));
    }
}

// ============================================================================
// SECTION 2: CONSTRUCTOR, DESTRUCTOR & INITIALIZATION
// ============================================================================

ClassSystem::ClassSystem() 
    : m_archetype(ClassArchetype::Wastelander),
      m_currentLevel(1),
      m_currentExperience(0),
      m_attributePoints(0),
      m_skillPoints(0),
      m_perkPoints(0)
{
    m_baseAttributes.clear();
    m_baseSkills.clear();
    m_unlockedPerks.clear();
    m_activeModifiers.clear();
    m_perkDatabase.clear();

    InitializePerkDatabase();
    InitializeDefaultStats();

    Platform::Log("[CLASS SYSTEM]: Подсистема RPG и атрибутов успешно загружена.");
}

ClassSystem::~ClassSystem() {
    m_baseAttributes.clear();
    m_baseSkills.clear();
    m_unlockedPerks.clear();
    m_activeModifiers.clear();
    m_perkDatabase.clear();
    Platform::Log("[CLASS SYSTEM]: Память RPG-подсистемы освобождена.");
}

// ============================================================================
// SECTION 3: PERK DATABASE & REQUIREMENTS VALIDATION
// ============================================================================

void ClassSystem::InitializePerkDatabase() {
    m_perkDatabase.clear();

    // Перк: Крепкая спина (Увеличивает переносимый вес)
    PerkDefinition pStrongBack;
    pStrongBack.perkId = 1001;
    pStrongBack.name = "Крепкая спина (Strong Back)";
    pStrongBack.description = "+50 кг к максимальному переносимому весу.";
    pStrongBack.reqLevel = 4;
    pStrongBack.reqAttributes[CharacterAttribute::Strength] = 6;
    m_perkDatabase[pStrongBack.perkId] = pStrongBack;

    // Перк: Снайпер (Увеличение шанса крита и урона в голову)
    PerkDefinition pSniper;
    pSniper.perkId = 1002;
    pSniper.name = "Снайпер (Sniper)";
    pSniper.description = "+15% к шансу критического попадания из винтовок.";
    pSniper.reqLevel = 8;
    pSniper.reqAttributes[CharacterAttribute::Perception] = 7;
    pSniper.reqAttributes[CharacterAttribute::Agility] = 6;
    pSniper.reqSkills[CharacterSkill::Firearms] = 50;
    m_perkDatabase[pSniper.perkId] = pSniper;

    // Перк: Желудок из свинца (Иммунитет к радиации от еды)
    PerkDefinition pLeadBelly;
    pLeadBelly.perkId = 1003;
    pLeadBelly.name = "Свинцовый желудок (Lead Belly)";
    pLeadBelly.description = "Вы больше не получаете радиацию при употреблении грязной воды и сырого мяса.";
    pLeadBelly.reqLevel = 2;
    pLeadBelly.reqAttributes[CharacterAttribute::Endurance] = 5;
    m_perkDatabase[pLeadBelly.perkId] = pLeadBelly;

    // Перк: Полевой медик
    PerkDefinition pMedic;
    pMedic.perkId = 1004;
    pMedic.name = "Полевой медик (Field Medic)";
    pMedic.description = "Стимпаки и бинты восстанавливают на 40% больше здоровья.";
    pMedic.reqLevel = 6;
    pMedic.reqAttributes[CharacterAttribute::Intelligence] = 6;
    pMedic.reqSkills[CharacterSkill::Medicine] = 40;
    m_perkDatabase[pMedic.perkId] = pMedic;

    // Перк: Кузнец-оружейник (Открывает крафт Tier 3)
    PerkDefinition pGunsmith;
    pGunsmith.perkId = 1005;
    pGunsmith.name = "Оружейник (Gunsmith)";
    pGunsmith.description = "Открывает доступ к продвинутым модификациям огнестрельного оружия на верстаке.";
    pGunsmith.reqLevel = 10;
    pGunsmith.reqAttributes[CharacterAttribute::Intelligence] = 7;
    pGunsmith.reqSkills[CharacterSkill::Mechanics] = 65;
    m_perkDatabase[pGunsmith.perkId] = pGunsmith;

    Platform::Log("[RPG DATABASE]: Дерево перков успешно сформировано. Доступно узлов: " + std::to_string(m_perkDatabase.size()));
}

void ClassSystem::InitializeDefaultStats() {
    // Инициализация атрибутов базовыми значениями (5 из 20)
    for (int i = 0; i < 7; ++i) {
        m_baseAttributes[static_cast<CharacterAttribute>(i)] = 5;
    }

    // Инициализация навыков нулями
    for (int i = 0; i < 7; ++i) {
        m_baseSkills[static_cast<CharacterSkill>(i)] = 10;
    }
}

// ============================================================================
// SECTION 4: ARCHETYPE SELECTION (NEW GAME CONFIGURATION)
// ============================================================================

void ClassSystem::SetArchetype(ClassArchetype archetype) {
    m_archetype = archetype;
    InitializeDefaultStats();

    Platform::Log(std::string("[RPG SYSTEM]: Применен классовый профиль: ") + ArchetypeToString(archetype));

    switch (archetype) {
        case ClassArchetype::Mercenary:
            m_baseAttributes[CharacterAttribute::Strength] += 2;
            m_baseAttributes[CharacterAttribute::Agility] += 1;
            m_baseSkills[CharacterSkill::Firearms] += 20;
            m_baseSkills[CharacterSkill::Melee] += 10;
            break;

        case ClassArchetype::Scavenger:
            m_baseAttributes[CharacterAttribute::Luck] += 3;
            m_baseAttributes[CharacterAttribute::Perception] += 1;
            m_baseSkills[CharacterSkill::Survival] += 25;
            m_baseSkills[CharacterSkill::Stealth] += 15;
            break;

        case ClassArchetype::TechPriest:
            m_baseAttributes[CharacterAttribute::Intelligence] += 4;
            m_baseAttributes[CharacterAttribute::Strength] -= 1;
            m_baseSkills[CharacterSkill::Mechanics] += 30;
            m_baseSkills[CharacterSkill::Medicine] += 10;
            break;

        case ClassArchetype::Raider:
            m_baseAttributes[CharacterAttribute::Endurance] += 3;
            m_baseAttributes[CharacterAttribute::Strength] += 1;
            m_baseAttributes[CharacterAttribute::Intelligence] -= 2;
            m_baseSkills[CharacterSkill::Melee] += 25;
            m_baseSkills[CharacterSkill::Survival] += 15;
            break;

        case ClassArchetype::Wastelander:
            // Базовый профиль "всего понемногу", без сильных отклонений
            m_baseAttributes[CharacterAttribute::Endurance] += 1;
            m_baseAttributes[CharacterAttribute::Luck] += 1;
            m_baseSkills[CharacterSkill::Survival] += 10;
            break;
    }

    RecalculateDerivedStats();
}

// ============================================================================
// SECTION 5: EXPERIENCE, LEVELING & PROGRESSION
// ============================================================================

void ClassSystem::AddExperience(uint32_t amount) {
    if (m_currentLevel >= MAX_CHARACTER_LEVEL) return;

    m_currentExperience += amount;
    Platform::Log("[RPG PROGRESS]: Получено +" + std::to_string(amount) + " XP. Всего: " + std::to_string(m_currentExperience));

    uint32_t xpRequired = CalculateRequiredExperienceForLevel(m_currentLevel + 1);

    while (m_currentExperience >= xpRequired && m_currentLevel < MAX_CHARACTER_LEVEL) {
        m_currentExperience -= xpRequired;
        LevelUp();
        xpRequired = CalculateRequiredExperienceForLevel(m_currentLevel + 1);
    }
}

void ClassSystem::LevelUp() {
    m_currentLevel++;
    
    // Начисление очков развития
    m_skillPoints += 15 + (GetTotalAttributeValue(CharacterAttribute::Intelligence) / 2);
    
    // Каждые 2 уровня даем очко Перков, каждые 5 уровней - очко Атрибутов S.P.E.C.I.A.L.
    if (m_currentLevel % 2 == 0) m_perkPoints++;
    if (m_currentLevel % 5 == 0) m_attributePoints++;

    Platform::Log("[LEVEL UP!]: Выживший достиг уровня " + std::to_string(m_currentLevel) + "!");
    RecalculateDerivedStats();
}

// ============================================================================
// SECTION 6: STAT ALLOCATION & PERK UNLOCKING
// ============================================================================

bool ClassSystem::AllocateAttributePoint(CharacterAttribute attr) {
    if (m_attributePoints == 0) return false;
    
    uint32_t currentValue = m_baseAttributes[attr];
    if (currentValue >= MAX_ATTRIBUTE_VALUE) {
        Platform::Log("[RPG ALLOCATION]: Достигнут максимум для данного атрибута (20).");
        return false;
    }

    m_baseAttributes[attr]++;
    m_attributePoints--;
    
    Platform::Log(std::string("[RPG SYSTEM]: Атрибут ") + AttributeToString(attr) + " увеличен до " + std::to_string(m_baseAttributes[attr]));
    RecalculateDerivedStats();
    return true;
}

bool ClassSystem::AllocateSkillPoints(CharacterSkill skill, uint32_t points) {
    if (m_skillPoints < points) return false;

    uint32_t currentValue = m_baseSkills[skill];
    if (currentValue + points > MAX_SKILL_VALUE) {
        uint32_t allowed = MAX_SKILL_VALUE - currentValue;
        m_baseSkills[skill] += allowed;
        m_skillPoints -= allowed;
    } else {
        m_baseSkills[skill] += points;
        m_skillPoints -= points;
    }

    Platform::Log(std::string("[RPG SYSTEM]: Навык ") + SkillToString(skill) + " повышен до " + std::to_string(m_baseSkills[skill]));
    return true;
}

bool ClassSystem::UnlockPerk(uint32_t perkId) {
    if (m_perkPoints == 0) {
        Platform::Log("[RPG ERROR]: Нет свободных очков перков (Perk Points).");
        return false;
    }

    if (HasPerk(perkId)) {
        Platform::Log("[RPG ERROR]: Данный перк уже разблокирован.");
        return false;
    }

    auto it = m_perkDatabase.find(perkId);
    if (it == m_perkDatabase.end()) return false;

    const PerkDefinition& perkDef = it->second;

    // Валидация требований по уровню
    if (m_currentLevel < perkDef.reqLevel) {
        Platform::Log("[RPG REQUIREMENT]: Недостаточный уровень. Требуется: " + std::to_string(perkDef.reqLevel));
        return false;
    }

    // Валидация требований по атрибутам S.P.E.C.I.A.L.
    for (const auto& [attr, reqValue] : perkDef.reqAttributes) {
        if (GetTotalAttributeValue(attr) < reqValue) {
            Platform::Log(std::string("[RPG REQUIREMENT]: Недостаточно атрибута ") + AttributeToString(attr) + 
                          ". Требуется: " + std::to_string(reqValue));
            return false;
        }
    }

    // Валидация требований по навыкам
    for (const auto& [skill, reqValue] : perkDef.reqSkills) {
        if (GetTotalSkillValue(skill) < reqValue) {
            Platform::Log(std::string("[RPG REQUIREMENT]: Недостаточно навыка ") + SkillToString(skill) + 
                          ". Требуется: " + std::to_string(reqValue));
            return false;
        }
    }

    // Разблокировка перка
    m_unlockedPerks.insert(perkId);
    m_perkPoints--;

    Platform::Log("[RPG PERK UNLOCKED]: Изучен новый перк: " + perkDef.name);
    RecalculateDerivedStats();
    return true;
}

bool ClassSystem::HasPerk(uint32_t perkId) const noexcept {
    return m_unlockedPerks.find(perkId) != m_unlockedPerks.end();
}

// ============================================================================
// SECTION 7: MODIFIERS (BUFFS & DEBUFFS) ENGINE
// ============================================================================

void ClassSystem::ApplyModifier(const StatModifier& modifier) {
    // Если это временный бафф (например, от "Винта" или "Психо"), и такой уже висит, обновляем таймер
    if (modifier.isTemporary) {
        auto it = std::find_if(m_activeModifiers.begin(), m_activeModifiers.end(),
            [&modifier](const StatModifier& m) { return m.sourceId == modifier.sourceId && m.isTemporary; });
        
        if (it != m_activeModifiers.end()) {
            it->durationRemainingSeconds = std::max(it->durationRemainingSeconds, modifier.durationRemainingSeconds);
            Platform::Log("[RPG MODIFIER]: Обновлено время действия эффекта ID: " + std::to_string(modifier.sourceId));
            return;
        }
    }

    m_activeModifiers.push_back(modifier);
    Platform::Log("[RPG MODIFIER]: Наложен новый эффект. ID: " + std::to_string(modifier.sourceId) + 
                  " (Значение: " + std::to_string(modifier.valueDelta) + ")");
    
    RecalculateDerivedStats();
}

void ClassSystem::RemoveModifierBySource(uint32_t sourceId) {
    auto it = std::remove_if(m_activeModifiers.begin(), m_activeModifiers.end(),
        [sourceId](const StatModifier& m) { return m.sourceId == sourceId; });
        
    if (it != m_activeModifiers.end()) {
        m_activeModifiers.erase(it, m_activeModifiers.end());
        Platform::Log("[RPG MODIFIER]: Эффект снят. ID: " + std::to_string(sourceId));
        RecalculateDerivedStats();
    }
}

void ClassSystem::UpdateModifiersTick(float deltaTime) {
    bool statChanged = false;

    for (auto it = m_activeModifiers.begin(); it != m_activeModifiers.end();) {
        if (it->isTemporary) {
            it->durationRemainingSeconds -= deltaTime;
            if (it->durationRemainingSeconds <= 0.0f) {
                Platform::Log("[RPG MODIFIER]: Истекло время действия временного эффекта ID: " + std::to_string(it->sourceId));
                it = m_activeModifiers.erase(it);
                statChanged = true;
                continue;
            }
        }
        ++it;
    }

    if (statChanged) {
        RecalculateDerivedStats();
    }
}

// ============================================================================
// SECTION 8: TOTAL STAT CALCULATIONS & DERIVED STATS
// ============================================================================

int32_t ClassSystem::GetTotalAttributeValue(CharacterAttribute attr) const {
    int32_t total = m_baseAttributes.at(attr);

    // Применяем модификаторы (от препаратов, брони, лучевой болезни)
    for (const auto& mod : m_activeModifiers) {
        if (mod.targetType == ModifierTarget::Attribute && mod.targetId == static_cast<uint32_t>(attr)) {
            total += mod.valueDelta;
        }
    }

    // Атрибуты не могут упасть ниже 1
    return std::max(1, total);
}

int32_t ClassSystem::GetTotalSkillValue(CharacterSkill skill) const {
    int32_t total = m_baseSkills.at(skill);

    // Добавляем синергию от главных атрибутов (Например, Firearms скейлится от Agility и Perception)
    switch (skill) {
        case CharacterSkill::Firearms:
            total += (GetTotalAttributeValue(CharacterAttribute::Agility) * 2) + GetTotalAttributeValue(CharacterAttribute::Perception);
            break;
        case CharacterSkill::Melee:
            total += (GetTotalAttributeValue(CharacterAttribute::Strength) * 3);
            break;
        case CharacterSkill::Survival:
            total += (GetTotalAttributeValue(CharacterAttribute::Endurance) * 2);
            break;
        case CharacterSkill::Medicine:
        case CharacterSkill::Mechanics:
            total += (GetTotalAttributeValue(CharacterAttribute::Intelligence) * 3);
            break;
        case CharacterSkill::Stealth:
            total += (GetTotalAttributeValue(CharacterAttribute::Agility) * 3);
            break;
        case CharacterSkill::Speech:
            total += (GetTotalAttributeValue(CharacterAttribute::Charisma) * 3);
            break;
    }

    // Применяем активные баффы/дебаффы к навыку
    for (const auto& mod : m_activeModifiers) {
        if (mod.targetType == ModifierTarget::Skill && mod.targetId == static_cast<uint32_t>(skill)) {
            total += mod.valueDelta;
        }
    }

    return std::clamp(total, 0, 100); // Навык заблокирован в пределах 0 - 100
}

void ClassSystem::RecalculateDerivedStats() {
    // 1. Максимальное Здоровье = База + (Уровень * 5) + (Выносливость * 10) + Бонус Силы
    m_derivedMaxHealth = 100.0f + (m_currentLevel * 5.0f) + (GetTotalAttributeValue(CharacterAttribute::Endurance) * 10.0f) + (GetTotalAttributeValue(CharacterAttribute::Strength) * 5.0f);

    // 2. Переносимый вес (Carry Weight) = 150 + (Сила * 15)
    m_derivedCarryWeight = 150.0f + (GetTotalAttributeValue(CharacterAttribute::Strength) * 15.0f);
    
    if (HasPerk(1001)) { // Крепкая спина
        m_derivedCarryWeight += 50.0f;
    }

    // 3. Очки Действия для V.A.T.S. / Стамина = 60 + (Ловкость * 10)
    m_derivedActionPoints = 60.0f + (GetTotalAttributeValue(CharacterAttribute::Agility) * 10.0f);

    // 4. Шанс Критического попадания = Удача * 1.5%
    m_derivedCritChance = GetTotalAttributeValue(CharacterAttribute::Luck) * 1.5f;

    // 5. Урон в ближнем бою = База + Сила * 2.5
    m_derivedMeleeDamageBonus = GetTotalAttributeValue(CharacterAttribute::Strength) * 2.5f;

    // Учет прямого воздействия модификаторов на производные статы
    for (const auto& mod : m_activeModifiers) {
        if (mod.targetType == ModifierTarget::DerivedStat) {
            switch (mod.targetId) {
                case 1: m_derivedMaxHealth += mod.valueDelta; break;     // ID 1 = Health
                case 2: m_derivedCarryWeight += mod.valueDelta; break;   // ID 2 = Carry Weight
                case 3: m_derivedActionPoints += mod.valueDelta; break;  // ID 3 = AP
            }
        }
    }

    Platform::Log("[RPG DERIVED]: Производные характеристики успешно пересчитаны.");
}

float ClassSystem::GetDerivedStat(uint32_t statId) const noexcept {
    switch (statId) {
        case 1: return m_derivedMaxHealth;
        case 2: return m_derivedCarryWeight;
        case 3: return m_derivedActionPoints;
        case 4: return m_derivedCritChance;
        case 5: return m_derivedMeleeDamageBonus;
        default: return 0.0f;
    }
}

// ============================================================================
// SECTION 9: BINARY SERIALIZATION WITH CRC32 (SAVE GAMES & NETWORKING)
// ============================================================================

uint32_t ClassSystem::CalculateChecksum(const std::vector<uint8_t>& buffer) const noexcept {
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : buffer) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

std::vector<uint8_t> ClassSystem::SerializeToBinary() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(1024);

    // Заголовок (Header)
    const uint8_t* magicPtr = reinterpret_cast<const uint8_t*>(&CLASS_SYSTEM_MAGIC);
    buffer.insert(buffer.end(), magicPtr, magicPtr + sizeof(uint32_t));

    const uint8_t* verPtr = reinterpret_cast<const uint8_t*>(&CLASS_SYSTEM_VERSION);
    buffer.insert(buffer.end(), verPtr, verPtr + sizeof(uint32_t));

    // Прогресс
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_archetype), reinterpret_cast<const uint8_t*>(&m_archetype) + sizeof(uint32_t));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_currentLevel), reinterpret_cast<const uint8_t*>(&m_currentLevel) + sizeof(uint32_t));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_currentExperience), reinterpret_cast<const uint8_t*>(&m_currentExperience) + sizeof(uint32_t));
    
    // Доступные очки развития
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_attributePoints), reinterpret_cast<const uint8_t*>(&m_attributePoints) + sizeof(uint32_t));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_skillPoints), reinterpret_cast<const uint8_t*>(&m_skillPoints) + sizeof(uint32_t));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_perkPoints), reinterpret_cast<const uint8_t*>(&m_perkPoints) + sizeof(uint32_t));

    // Базовые атрибуты (S.P.E.C.I.A.L.)
    uint32_t attrCount = static_cast<uint32_t>(m_baseAttributes.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&attrCount), reinterpret_cast<const uint8_t*>(&attrCount) + sizeof(uint32_t));
    for (const auto& [attr, value] : m_baseAttributes) {
        uint32_t aId = static_cast<uint32_t>(attr);
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&aId), reinterpret_cast<const uint8_t*>(&aId) + sizeof(uint32_t));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&value), reinterpret_cast<const uint8_t*>(&value) + sizeof(uint32_t));
    }

    // Базовые навыки
    uint32_t skillCount = static_cast<uint32_t>(m_baseSkills.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&skillCount), reinterpret_cast<const uint8_t*>(&skillCount) + sizeof(uint32_t));
    for (const auto& [skill, value] : m_baseSkills) {
        uint32_t sId = static_cast<uint32_t>(skill);
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&sId), reinterpret_cast<const uint8_t*>(&sId) + sizeof(uint32_t));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&value), reinterpret_cast<const uint8_t*>(&value) + sizeof(uint32_t));
    }

    // Разблокированные перки
    uint32_t perkCount = static_cast<uint32_t>(m_unlockedPerks.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&perkCount), reinterpret_cast<const uint8_t*>(&perkCount) + sizeof(uint32_t));
    for (uint32_t pId : m_unlockedPerks) {
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&pId), reinterpret_cast<const uint8_t*>(&pId) + sizeof(uint32_t));
    }

    // Сохранение постоянных модификаторов (временные баффы в сейв не пишутся)
    std::vector<StatModifier> permanentMods;
    for (const auto& mod : m_activeModifiers) {
        if (!mod.isTemporary) permanentMods.push_back(mod);
    }
    
    uint32_t modCount = static_cast<uint32_t>(permanentMods.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&modCount), reinterpret_cast<const uint8_t*>(&modCount) + sizeof(uint32_t));
    for (const auto& mod : permanentMods) {
        const uint8_t* mPtr = reinterpret_cast<const uint8_t*>(&mod);
        buffer.insert(buffer.end(), mPtr, mPtr + sizeof(StatModifier));
    }

    // Контрольная сумма пакета
    uint32_t checksum = CalculateChecksum(buffer);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&checksum), reinterpret_cast<const uint8_t*>(&checksum) + sizeof(uint32_t));

    Platform::Log("[RPG SERIALIZE]: Структура класса игрока скомпилирована в бинарный пакет (" + std::to_string(buffer.size()) + " байт).");
    return buffer;
}

bool ClassSystem::DeserializeFromBinary(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(uint32_t) * 10) {
        Platform::Log("[RPG DESERIALIZE ERROR]: Ошибка дампа. Слишком малый размер бинарного буфера.");
        return false;
    }

    size_t payloadSize = buffer.size() - sizeof(uint32_t);
    std::vector<uint8_t> payloadData(buffer.begin(), buffer.begin() + payloadSize);
    uint32_t expectedChecksum = CalculateChecksum(payloadData);

    uint32_t storedChecksum = 0;
    std::memcpy(&storedChecksum, buffer.data() + payloadSize, sizeof(uint32_t));

    if (expectedChecksum != storedChecksum) {
        Platform::Log("[RPG DESERIALIZE ERROR]: Искажение данных профиля (CRC32 Mismatch). Возможна попытка редактирования сейва!");
        return false;
    }

    size_t cursor = 0;
    uint32_t magic = 0;
    std::memcpy(&magic, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    if (magic != CLASS_SYSTEM_MAGIC) return false;

    uint32_t version = 0;
    std::memcpy(&version, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

    std::memcpy(&m_archetype, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    std::memcpy(&m_currentLevel, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    std::memcpy(&m_currentExperience, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    
    std::memcpy(&m_attributePoints, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    std::memcpy(&m_skillPoints, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    std::memcpy(&m_perkPoints, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

    uint32_t attrCount = 0;
    std::memcpy(&attrCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    m_baseAttributes.clear();
    for (uint32_t i = 0; i < attrCount; ++i) {
        uint32_t aId, value;
        std::memcpy(&aId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        std::memcpy(&value, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        m_baseAttributes[static_cast<CharacterAttribute>(aId)] = value;
    }

    uint32_t skillCount = 0;
    std::memcpy(&skillCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    m_baseSkills.clear();
    for (uint32_t i = 0; i < skillCount; ++i) {
        uint32_t sId, value;
        std::memcpy(&sId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        std::memcpy(&value, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        m_baseSkills[static_cast<CharacterSkill>(sId)] = value;
    }

    uint32_t perkCount = 0;
    std::memcpy(&perkCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    m_unlockedPerks.clear();
    for (uint32_t i = 0; i < perkCount; ++i) {
        uint32_t pId;
        std::memcpy(&pId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        m_unlockedPerks.insert(pId);
    }

    uint32_t modCount = 0;
    std::memcpy(&modCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    m_activeModifiers.clear();
    for (uint32_t i = 0; i < modCount; ++i) {
        StatModifier mod;
        std::memcpy(&mod, buffer.data() + cursor, sizeof(StatModifier)); cursor += sizeof(StatModifier);
        m_activeModifiers.push_back(mod);
    }

    RecalculateDerivedStats();
    Platform::Log("[RPG DESERIALIZE]: Данные профиля и атрибуты успешно восстановлены. Уровень персонажа: " + std::to_string(m_currentLevel));
    return true;
}

} // namespace Centralia