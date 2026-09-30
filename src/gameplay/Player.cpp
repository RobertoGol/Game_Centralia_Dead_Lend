#include "gameplay/Player.hpp"
#include "gameplay/WeaponSystem.hpp"
#include "gameplay/PowerArmorStateData.hpp"
#include "core/MemoryManager.hpp"
#include "platform/Platform.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTRUCTOR, DESTRUCTOR & INITIALIZATION
// ============================================================================

Player::Player() 
    : m_playerId(1001),
      m_playerName("Survivor_Zero"),
      m_position(0.0f, 0.0f, 0.0f),
      m_velocity(0.0f, 0.0f, 0.0f),
      m_rotationYaw(0.0f),
      m_rotationPitch(0.0f),
      m_maxHealth(100.0f),
      m_currentHealth(100.0f),
      m_maxStamina(100.0f),
      m_currentStamina(100.0f),
      m_radiationLevel(0.0f),
      m_hungerLevel(0.0f),
      m_thirstLevel(0.0f),
      m_maxCarryWeight(150.0f),
      m_currentInventoryWeight(0.0f),
      m_isSprinting(false),
      m_isCrouching(false),
      m_isInPowerArmor(false),
      m_level(1),
      m_experiencePoints(0)
{
    m_inventory.clear();
    m_inventory.reserve(64); // Резервируем память под 64 ячейки инвентаря
    
    // Выдаем стартовый набор выжившего в Пустоши
    AddItemToInventory(600, 24); // 9mm патроны
    AddItemToInventory(2001, 10); // Древесина
    AddItemToInventory(3001, 2);  // Стимпаки

    Platform::Log("[PLAYER SYSTEM]: Survivor instance successfully instantiated in RAM.");
}

Player::~Player() {
    m_inventory.clear();
    Platform::Log("[PLAYER SYSTEM]: Survivor instance destroyed and memory safely unmapped.");
}

// ============================================================================
// SECTION 2: MOVEMENT, KINEMATICS & STAMINA MANAGEMENT
// ============================================================================

void Player::UpdateMovement(float deltaTime, const Vector3D& inputMoveVector, bool sprintPressed, bool crouchPressed) {
    m_isCrouching = crouchPressed;
    
    float baseSpeed = m_isCrouching ? 2.2f : 5.5f;
    
    // Если игрок в силовой броне, модифицируем скорость через экзоскелет
    if (m_isInPowerArmor) {
        baseSpeed = m_isCrouching ? 1.8f : 6.8f;
    }

    // Обработка спринта и выносливости (Stamina)
    if (sprintPressed && !m_isCrouching && inputMoveVector.Length() > 0.1f && m_currentStamina > 5.0f) {
        m_isSprinting = true;
        baseSpeed *= 1.6f;
        m_currentStamina -= 18.0f * deltaTime;
        if (m_currentStamina < 0.0f) {
            m_currentStamina = 0.0f;
            m_isSprinting = false;
        }
    } else {
        m_isSprinting = false;
        // Восстановление выносливости в покое
        if (m_currentStamina < m_maxStamina) {
            m_currentStamina += 12.0f * deltaTime;
            if (m_currentStamina > m_maxStamina) m_currentStamina = m_maxStamina;
        }
    }

    // Интеграция перемещения в 3D пространстве
    Vector3D normalizedInput = inputMoveVector.Length() > 0.0f ? inputMoveVector.Normalized() : Vector3D(0,0,0);
    m_velocity = normalizedInput * baseSpeed;
    m_position = m_position + (m_velocity * deltaTime);
}

void Player::SetRotation(float yaw, float pitch) noexcept {
    m_rotationYaw = yaw;
    m_rotationPitch = std::clamp(pitch, -89.0f, 89.0f); // Ограничение угла обзора по вертикали
}

const Vector3D& Player::GetPosition() const noexcept {
    return m_position;
}

void Player::Teleport(const Vector3D& newPosition) noexcept {
    m_position = newPosition;
    Platform::Log("[PLAYER KINEMATICS]: Player teleported to coordinates X: " + 
                  std::to_string(newPosition.x) + " Y: " + std::to_string(newPosition.y) + " Z: " + std::to_string(newPosition.z));
}

// ============================================================================
// SECTION 3: HEALTH, SURVIVAL METRICS & RADIATION SICKNESS
// ============================================================================

void Player::ApplyDamage(float damageAmount) noexcept {
    if (damageAmount <= 0.0f) return;

    float netDamage = damageAmount;
    
    // Если надет экзоскелет силовой брони, урон сначала гасится броней
    if (m_isInPowerArmor) {
        netDamage *= 0.35f; // Силовая броня поглощает 65% входящего урона
    }

    m_currentHealth -= netDamage;
    if (m_currentHealth <= 0.0f) {
        m_currentHealth = 0.0f;
        Platform::Log("[PLAYER COMBAT]: Survivor has fallen unconscious / died in the Wasteland.");
    } else {
        Platform::Log("[PLAYER COMBAT]: Survivor took " + std::to_string(netDamage) + " damage. HP left: " + std::to_string(m_currentHealth));
    }
}

void Player::Heal(float healAmount) noexcept {
    if (healAmount <= 0.0f || m_currentHealth <= 0.0f) return;

    m_currentHealth += healAmount;
    if (m_currentHealth > m_maxHealth) {
        m_currentHealth = m_maxHealth;
    }
    Platform::Log("[PLAYER SURVIVAL]: Survivor healed by " + std::to_string(healAmount) + ". Current HP: " + std::to_string(m_currentHealth));
}

void Player::ApplyRadiation(float radAmount) noexcept {
    if (radAmount <= 0.0f) return;

    m_radiationLevel += radAmount;
    if (m_radiationLevel > 1000.0f) m_radiationLevel = 1000.0f;

    // Радиация снижает максимальный запас здоровья выжившего
    float healthPenalty = (m_radiationLevel / 1000.0f) * (m_maxHealth * 0.5f);
    if (m_currentHealth > (m_maxHealth - healthPenalty)) {
        m_currentHealth = m_maxHealth - healthPenalty;
    }

    Platform::Log("[PLAYER HAZARD]: Radiation accumulated: +" + std::to_string(radAmount) + " RAD. Total: " + std::to_string(m_radiationLevel));
}

void Player::UpdateSurvivalTicks(float deltaTime) noexcept {
    // Постепенное увеличение голода и жажды
    m_hungerLevel += 0.05f * deltaTime;
    m_thirstLevel += 0.08f * deltaTime;

    if (m_hungerLevel >= 100.0f) {
        m_hungerLevel = 100.0f;
        ApplyDamage(1.5f * deltaTime); // Истощение от голода наносит урон
    }

    if (m_thirstLevel >= 100.0f) {
        m_thirstLevel = 100.0f;
        ApplyDamage(2.5f * deltaTime); // Обезвоживание убивает быстрее
    }
}

// ============================================================================
// SECTION 4: INVENTORY, CARRY WEIGHT & ITEM MANAGEMENT
// ============================================================================

bool Player::AddItemToInventory(uint32_t itemId, uint16_t quantity) {
    if (quantity == 0) return false;

    // Ищем, есть ли уже такой предмет в инвентаре для стакинга
    for (auto& item : m_inventory) {
        if (item.id == itemId) {
            item.quantity += quantity;
            RecalculateInventoryWeight();
            Platform::Log("[INVENTORY]: Stacked " + std::to_string(quantity) + " units of item ID " + std::to_string(itemId));
            return true;
        }
    }

    // Если слотов меньше 64 и вес позволяет — добавляем новый слот
    if (m_inventory.size() >= 64) {
        Platform::Log("[INVENTORY ERROR]: Inventory capacity limit reached (64 slots max).");
        return false;
    }

    InventoryItem newItem;
    newItem.id = itemId;
    newItem.quantity = quantity;
    newItem.weightPerUnit = 0.5f; // Стандартный вес единицы предмета по умолчанию

    m_inventory.push_back(newItem);
    RecalculateInventoryWeight();
    
    Platform::Log("[INVENTORY]: Added new item ID " + std::to_string(itemId) + " (Qty: " + std::to_string(quantity) + ")");
    return true;
}

bool Player::RemoveItem(size_t slotIndex, uint16_t quantity) {
    if (slotIndex >= m_inventory.size()) return false;

    auto& item = m_inventory[slotIndex];
    if (item.quantity <= quantity) {
        // Удаляем весь слот целиком, если количество исчерпано
        m_inventory.erase(m_inventory.begin() + slotIndex);
        Platform::Log("[INVENTORY]: Item slot " + std::to_string(slotIndex) + " fully depleted and removed.");
    } else {
        item.quantity -= quantity;
        Platform::Log("[INVENTORY]: Removed " + std::to_string(quantity) + " units from slot " + std::to_string(slotIndex));
    }

    RecalculateInventoryWeight();
    return true;
}

const std::vector<InventoryItem>& Player::GetInventory() const noexcept {
    return m_inventory;
}

void Player::RecalculateInventoryWeight() noexcept {
    float totalWeight = 0.0f;
    for (const auto& item : m_inventory) {
        totalWeight += static_cast<float>(item.quantity) * item.weightPerUnit;
    }
    m_currentInventoryWeight = totalWeight;

-    // Проверка перегруза (Overencumbered)
    if (m_currentInventoryWeight > m_maxCarryWeight) {
        // Перегруз замедляет игрока
    }
}

float Player::GetCurrentInventoryWeight() const noexcept {
    return m_currentInventoryWeight;
}

// ============================================================================
// SECTION 5: POWER ARMOR INTEGRATION
// ============================================================================

void Player::EnterPowerArmor() noexcept {
    m_isInPowerArmor = true;
    m_maxCarryWeight += 250.0f; // Экзоскелет дает гигантский бонус переносимого веса
    Platform::Log("[PLAYER SUIT]: Survivor successfully mounted into Power Armor exoskeleton.");
}

void Player::ExitPowerArmor() noexcept {
    m_isInPowerArmor = false;
    m_maxCarryWeight -= 250.0f;
    if (m_maxCarryWeight < 100.0f) m_maxCarryWeight = 100.0f;
    Platform::Log("[PLAYER SUIT]: Survivor dismounted from Power Armor exoskeleton.");
}

bool Player::IsInPowerArmor() const noexcept {
    return m_isInPowerArmor;
}

// ============================================================================
// SECTION 6: EXPERIENCE, STATS & LEVELING
// ============================================================================

void Player::AddExperience(uint32_t expAmount) noexcept {
    m_experiencePoints += expAmount;
    Platform::Log("[PLAYER PROGRESS]: Gained +" + std::to_string(expAmount) + " XP. Total: " + std::to_string(m_experiencePoints));

    uint32_t requiredForNextLevel = m_level * 1000;
    if (m_experiencePoints >= requiredForNextLevel) {
        m_level++;
        m_maxHealth += 15.0f;
        m_currentHealth = m_maxHealth;
        m_maxStamina += 10.0f;
        Platform::Log("[LEVEL UP!]: Survivor reached level " + std::to_string(m_level) + "! Stats upgraded.");
    }
}

uint32_t Player::GetLevel() const noexcept {
    return m_level;
}

// ============================================================================
// SECTION 7: BINARY SERIALIZATION FOR SAVEGAMES (GHOST-RAM / DISK)
// ============================================================================

std::vector<uint8_t> Player::SerializeToBinary() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(1024);

    // Упаковываем ID и имя
    const uint8_t* idPtr = reinterpret_cast<const uint8_t*>(&m_playerId);
    buffer.insert(buffer.end(), idPtr, idPtr + sizeof(uint32_t));

    uint32_t nameLen = static_cast<uint32_t>(m_playerName.size());
    const uint8_t* lenPtr = reinterpret_cast<const uint8_t*>(&nameLen);
    buffer.insert(buffer.end(), lenPtr, lenPtr + sizeof(uint32_t));
    buffer.insert(buffer.end(), m_playerName.begin(), m_playerName.end());

    // Упаковываем позицию (Vector3D)
    const uint8_t* posPtr = reinterpret_cast<const uint8_t*>(&m_position);
    buffer.insert(buffer.end(), posPtr, posPtr + sizeof(Vector3D));

    // Упаковываем здоровье, стамину и радиацию
    const uint8_t* statsPtr = reinterpret_cast<const uint8_t*>(&m_currentHealth);
    buffer.insert(buffer.end(), statsPtr, statsPtr + sizeof(float));
    
    const uint8_t* stamPtr = reinterpret_cast<const uint8_t*>(&m_currentStamina);
    buffer.insert(buffer.end(), stamPtr, stamPtr + sizeof(float));

    const uint8_t* radPtr = reinterpret_cast<const uint8_t*>(&m_radiationLevel);
    buffer.insert(buffer.end(), radPtr, radPtr + sizeof(float));

    // Упаковываем инвентарь
    uint32_t invSize = static_cast<uint32_t>(m_inventory.size());
    const uint8_t* invSizePtr = reinterpret_cast<const uint8_t*>(&invSize);
    buffer.insert(buffer.end(), invSizePtr, invSizePtr + sizeof(uint32_t));

    for (const auto& item : m_inventory) {
        const uint8_t* itemPtr = reinterpret_cast<const uint8_t*>(&item);
        buffer.insert(buffer.end(), itemPtr, itemPtr + sizeof(InventoryItem));
    }

    Platform::Log("[PLAYER SAVE]: Player state successfully serialized into binary stream (" + std::to_string(buffer.size()) + " bytes).");
    return buffer;
}

bool Player::DeserializeFromBinary(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(uint32_t) * 3) {
        Platform::Log("[PLAYER LOAD ERROR]: Binary stream too short for player deserialization.");
        return false;
    }

    size_t cursor = 0;

    std::memcpy(&m_playerId, buffer.data() + cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    uint32_t nameLen = 0;
    std::memcpy(&nameLen, buffer.data() + cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    if (cursor + nameLen > buffer.size()) return false;
    m_playerName.assign(reinterpret_cast<const char*>(buffer.data() + cursor), nameLen);
    cursor += nameLen;

    if (cursor + sizeof(Vector3D) > buffer.size()) return false;
    std::memcpy(&m_position, buffer.data() + cursor, sizeof(Vector3D));
    cursor += sizeof(Vector3D);

    if (cursor + sizeof(float) * 3 > buffer.size()) return false;
    std::memcpy(&m_currentHealth, buffer.data() + cursor, sizeof(float));
    cursor += sizeof(float);
    std::memcpy(&m_currentStamina, buffer.data() + cursor, sizeof(float));
    cursor += sizeof(float);
    std::memcpy(&m_radiationLevel, buffer.data() + cursor, sizeof(float));
    cursor += sizeof(float);

    if (cursor + sizeof(uint32_t) > buffer.size()) return false;
    uint32_t invSize = 0;
    std::memcpy(&invSize, buffer.data() + cursor, sizeof(uint32_t));
    cursor += sizeof(uint32_t);

    m_inventory.clear();
    for (uint32_t i = 0; i < invSize; ++i) {
        if (cursor + sizeof(InventoryItem) > buffer.size()) return false;
        InventoryItem item;
        std::memcpy(&item, buffer.data() + cursor, sizeof(InventoryItem));
        m_inventory.push_back(item);
        cursor += sizeof(InventoryItem);
    }

    RecalculateInventoryWeight();
    Platform::Log("[PLAYER LOAD]: Player state successfully restored from binary save dump.");
    return true;
}

} // namespace Centralia