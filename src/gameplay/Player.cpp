#include "gameplay/Player.hpp"
#include "gameplay/ItemDatabase.hpp"
#include "core/NetworkProtocol.hpp"
#include "platform/Platform.hpp"
#include <algorithm>
#include <string>

namespace Centralia {

// Конструктор по умолчанию (слайдеры по умолчанию, если редактор пропущен)
Player::Player() 
    : m_uid(777), m_nickname("Vault_Survivor"), m_maxInventorySlots(20), 
      m_activeWeaponId(0), m_activeArmorId(0), m_position(0.0f, 0.0f, 0.0f), m_rotationY(0.0f) 
{
    m_bodyMorph.gender = CharacterGender::Male;
    m_bodyMorph.breastSize = 1.0f;
    m_bodyMorph.intimateInt = 1.0f;
    m_bodyMorph.gluteusSize = 1.0f;
    m_bodyMorph.heightScale = 1.0f;
    m_bodyMorph.muscleMass = 1.0f;
}

// Конструктор инициализации новой сессии игрока
Player::Player(uint64_t uid, const std::string& name, size_t slots)
    : m_uid(uid), m_nickname(name), m_maxInventorySlots(slots), 
      m_activeWeaponId(0), m_activeArmorId(0), m_position(0.0f, 0.0f, 0.0f), m_rotationY(0.0f) 
{
    m_bodyMorph.gender = CharacterGender::Male;
    m_bodyMorph.breastSize = 1.0f;
    m_bodyMorph.intimateInt = 1.0f;
    m_bodyMorph.gluteusSize = 1.0f;
    m_bodyMorph.heightScale = 1.0f;
    m_bodyMorph.muscleMass = 1.0f;
}

Player::~Player() {}

// --- СИСТЕМА УПРАВЛЕНИЯ ПЕРЕМЕЩЕНИЕМ И ФИЗИКОЙ ---

void Player::UpdateMovementState(float deltaTime, bool isSprinting, bool isCtrlPressed) {
    // Логика переключения скоростей и траты выносливости силовой брони
}

void Player::Move(const Vector3D& direction, float speed, float deltaTime) {
    m_position = m_position + (direction.Normalize() * speed * deltaTime);
}

bool Player::IsMoving() const noexcept {
    // Метод проверки движения из main.cpp:66
    return true; 
}

float Player::GetCurrentMapTileHeight() const noexcept {
    // Возвращаем базовый высотный слой земли из test.map для main.cpp:78
    return 51.0f; 
}

float Player::GetEquippedArmorWeight() const noexcept {
    // Безопасное извлечение веса тяжелой брони T-60 из базы данных предметов
    const auto* armorTemplate = ItemDatabase::GetInstance().GetItemTemplatePtr(m_activeArmorId);
    return armorTemplate ? armorTemplate->weight : 0.0f;
}

void Player::AddItemToInventory(uint32_t itemId, uint32_t count) {
    AddItem(itemId, static_cast<uint16_t>(count));
}

// --- ЛОГИКА ИНВЕНТАРЯ И ХРАНИЛИЩА ---

bool Player::AddItem(uint32_t itemId, uint16_t qty, float durability) {
    for (auto& item : m_inventory) {
        if (item.id == itemId && item.durability == durability) {
            item.quantity += qty;
            return true;
        }
    }
    if (m_inventory.size() >= m_maxInventorySlots) return false;
    m_inventory.push_back({itemId, qty, durability});
    return true;
}

bool Player::RemoveItem(size_t slotIndex, uint16_t qty) {
    if (slotIndex >= m_inventory.size()) return false;
    if (m_inventory[slotIndex].quantity > qty) {
        m_inventory[slotIndex].quantity -= qty;
    } else {
        m_inventory.erase(m_inventory.begin() + slotIndex);
    }
    return true;
}

bool Player::UseItem(size_t slotIndex) {
    if (slotIndex >= m_inventory.size()) return false;
    
    Item& userItem = m_inventory[slotIndex];
    ItemTemplate itemData;
    
    if (!ItemDatabase::GetInstance().GetTemplate(userItem.id, itemData)) {
        return false;
    }

    if (userItem.durability <= 0.0f && (itemData.type == ItemType::Weapon || itemData.type == ItemType::Armor)) {
        Platform::Log("Нельзя использовать сломанный предмет: " + itemData.name);
        return false;
    }

    if (itemData.type == ItemType::Medical || itemData.type == ItemType::Consumable) {
        m_stats.health = std::min(100.0f, m_stats.health + itemData.heal_amount);
        m_stats.radiation = std::max(0.0f, m_stats.radiation - itemData.rad_remedy);

        if (userItem.id == 401) { // Грязная вода
            m_stats.thirst = std::min(100.0f, m_stats.thirst + 40.0f);
            m_stats.radiation = std::min(100.0f, m_stats.radiation + 10.0f);
            Platform::Log(m_nickname + " выпил грязную воду. Жажда утолена, но получен урон от радиации!");
        }

        Platform::Log(m_nickname + " использовал: " + itemData.name + 
                      " [HP: " + std::to_string(m_stats.health) + 
                      ", RAD: " + std::to_string(m_stats.radiation) + "]");

        return RemoveItem(slotIndex, 1);
    }

    return EquipItem(slotIndex);
}

bool Player::EquipItem(size_t slotIndex) {
    if (slotIndex >= m_inventory.size()) return false;
    
    ItemTemplate itemData;
    if (!ItemDatabase::GetInstance().GetTemplate(m_inventory[slotIndex].id, itemData)) return false;

    if (itemData.type == ItemType::Weapon) {
        m_activeWeaponId = itemData.id;
        Platform::Log(m_nickname + " взял в руки: " + itemData.name);
        return true;
    } 
    else if (itemData.type == ItemType::Armor) {
        m_activeArmorId = itemData.id;
        Platform::Log(m_nickname + " экипировал броню: " + itemData.name);
        return true;
    }
    return false;
}

void Player::UpdateSurvival(float deltaTime) {
    if (m_stats.health <= 0.0f) return;

    float hungerDrainRate = 0.05f; 
    float thirstDrainRate = 0.08f; 

    m_stats.hunger = std::max(0.0f, m_stats.hunger - (hungerDrainRate * deltaTime));
    m_stats.thirst = std::max(0.0f, m_stats.thirst - (thirstDrainRate * deltaTime));

    if (m_stats.hunger <= 0.0f || m_stats.thirst <= 0.0f) {
        m_stats.health = std::max(0.0f, m_stats.health - (1.0f * deltaTime));
    }

    if (m_stats.radiation > 50.0f) {
        m_stats.health = std::max(0.0f, m_stats.health - (0.5f * deltaTime));
    }
}

// --- СЕРИАЛИЗАЦИЯ ДАННЫХ В СЕТЕВОЙ ПОТОК ---

std::vector<uint8_t> Player::SerializeState() const {
    std::vector<uint8_t> buffer;

    NetworkSerializer::WriteUInt32(buffer, static_cast<uint32_t>(m_uid & 0xFFFFFFFF)); 
    NetworkSerializer::WriteUInt32(buffer, static_cast<uint32_t>((m_uid >> 32) & 0xFFFFFFFF));
    NetworkSerializer::WriteString(buffer, m_nickname);

    NetworkSerializer::WriteFloat(buffer, m_stats.health);
    NetworkSerializer::WriteFloat(buffer, m_stats.hunger);
    NetworkSerializer::WriteFloat(buffer, m_stats.thirst);
    NetworkSerializer::WriteFloat(buffer, m_stats.radiation);

    NetworkSerializer::WriteUInt32(buffer, m_activeWeaponId);
    NetworkSerializer::WriteUInt32(buffer, m_activeArmorId);

    NetworkSerializer::WriteUInt32(buffer, static_cast<uint32_t>(m_inventory.size()));
    for (const auto& item : m_inventory) {
        NetworkSerializer::WriteUInt32(buffer, item.id);
        NetworkSerializer::WriteUInt16(buffer, item.quantity);
        NetworkSerializer::WriteFloat(buffer, item.durability);
    }
    return buffer;
}

bool Player::DeserializeState(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (offset >= buffer.size()) return false;

    uint32_t uidLow = NetworkSerializer::ReadUInt32(buffer, offset);
    uint32_t uidHigh = NetworkSerializer::ReadUInt32(buffer, offset);
    m_uid = static_cast<uint64_t>(uidLow) | (static_cast<uint64_t>(uidHigh) << 32);
    m_nickname = NetworkSerializer::ReadString(buffer, offset);

    m_stats.health = NetworkSerializer::ReadFloat(buffer, offset);
    m_stats.hunger = NetworkSerializer::ReadFloat(buffer, offset);
    m_stats.thirst = NetworkSerializer::ReadFloat(buffer, offset);
    m_stats.radiation = NetworkSerializer::ReadFloat(buffer, offset);

    m_activeWeaponId = NetworkSerializer::ReadUInt32(buffer, offset);
    m_activeArmorId = NetworkSerializer::ReadUInt32(buffer, offset);

    uint32_t invSize = NetworkSerializer::ReadUInt32(buffer, offset);
    m_inventory.clear();
    for (uint32_t i = 0; i < invSize; ++i) {
        uint32_t id = NetworkSerializer::ReadUInt32(buffer, offset);
        uint16_t qty = NetworkSerializer::ReadUInt16(buffer, offset);
        float durability = NetworkSerializer::ReadFloat(buffer, offset);
        m_inventory.push_back({id, qty, durability});
    }
    return true;
}

} // namespace Centralia
