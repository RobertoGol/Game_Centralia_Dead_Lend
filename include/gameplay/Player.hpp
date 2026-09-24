#pragma once
#include "core/Math3D.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace Centralia {

// Категории предметов в мире Dead Lend (база из LDoE_Engine и твоих дизайн-доков)
enum class ItemType : uint8_t {
    Weapon     = 0,
    Armor      = 1,
    Medical    = 2,
    Resource   = 3,
    Consumable = 4
};

// Структура одного предмета в инвентаре
struct Item {
    uint32_t id;          // ID типа предмета
    uint16_t quantity;    // Количество в одной ячейке (стак)
    float durability;     // Прочность предмета от 0.0f (сломан) до 1.0f (новый)
};  

// Жизненные показатели выжившего в Centralia
struct SurvivalStats {
    float health    = 100.0f; // Здоровье (0.0f — смерть)
    float hunger    = 100.0f; // Сытость (0.0f — истощение)
    float thirst    = 100.0f; // Жажда
    float radiation = 0.0f;   // Накопленное заражение (100.0f — смертельная доза)
};

class Player {
private:
    uint64_t m_uid;                 // Уникальный сетевой ID игрока
    std::string m_nickname;         // Имя персонажа
    SurvivalStats m_stats;          // Текущие статы
    std::vector<Item> m_inventory;  // Сетка инвентаря
    size_t m_maxInventorySlots;     // Ограничение по слотам

    // Слоты для экипировки (хранят ID предметов, 0 — пусто)
    uint32_t m_activeWeaponId;
    uint32_t m_activeArmorId;

    Vector3D m_position;
    float m_rotationY = 0.0f; // Поворот персонажа вокруг вертикальной оси


public:
    Player(uint64_t uid, const std::string& name, size_t slots = 20);
    ~Player();

    const Vector3D& GetPosition() const { return m_position; }
    void SetPosition(const Vector3D& pos) { m_position = pos; }
    
    float GetRotation() const { return m_rotationY; }
    void SetRotation(float angle) { m_rotationY = angle; }

    void Move(const Vector3D& direction, float speed, float deltaTime) {
        m_position = m_position + (direction.Normalize() * speed * deltaTime);
    }


    // Геттеры и сеттеры для механик выживания
    uint64_t GetUID() const { return m_uid; }
    const std::string& GetNickname() const { return m_nickname; }
    SurvivalStats& GetStats() { return m_stats; }
    
    // Логика инвентаря
    bool AddItem(uint32_t itemId, uint16_t qty, float durability = 1.0f);
    bool RemoveItem(size_t slotIndex, uint16_t qty);
    const std::vector<Item>& GetInventory() const { return m_inventory; }

    // Использовать предмет из инвентаря по его индексу (аптечка, вода и т.д.)
    bool UseItem(size_t slotIndex);

    // Экипировать оружие или броню
    bool EquipItem(size_t slotIndex);

    // Пошаговый обсчет выживания (вызывается в игровом цикле)
    void UpdateSurvival(float deltaTime);

    uint32_t GetActiveWeaponId() const { return m_activeWeaponId; }
    uint32_t GetActiveArmorId() const { return m_activeArmorId; }

    // Кроссплатформенная сериализация состояния игрока в байты (для сети или сохранений)
    std::vector<uint8_t> SerializeState() const;
    bool DeserializeState(const std::vector<uint8_t>& buffer, size_t& offset);
};

} // namespace Centralia
