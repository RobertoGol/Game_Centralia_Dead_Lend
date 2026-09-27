#pragma once
#include "core/Math3D.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace Centralia {

// Пол персонажа для инициализации 3D-модели шасси
enum class CharacterGender : uint8_t {
    Male,
    Female
};

#pragma pack(push, 1)
// Высокоточная бинарная структура слайдеров редактора (Морфинг Сетки Тела)
struct BodyMorphStats {
    CharacterGender gender;
    
    // Слайдеры пропорций скелета [значения от 0.0f до 2.0f]
    float breastSize;        // Размер груди (Female модификатор)
    float intimateInt;       // Размер интимной зоны / "болта" (Male модификатор)
    float gluteusSize;       // Размер ягодиц / таза
    float heightScale;       // Рост гуманоида
    float muscleMass;        // Плотность мышечной массы
};
#pragma pack(pop)

// Категории предметов в мире Dead Lend
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
    float m_rotationY;              // Поворот персонажа вокруг вертикальной оси
    BodyMorphStats m_bodyMorph;     // Хранит точные слайдеры анатомии для GPU-шейдера

public:
    // Конструкторы и деструктор
    Player();
    Player(uint64_t uid, const std::string& name, size_t slots = 20);
    ~Player();

    // Константные и инлайн-совместимые геттеры/сеттеры
    [[nodiscard]] uint32_t GetActiveArmorId() const noexcept { return m_activeArmorId; }
    [[nodiscard]] uint32_t GetActiveWeaponId() const noexcept { return m_activeWeaponId; }
    [[nodiscard]] const BodyMorphStats& GetBodyMorph() const noexcept { return m_bodyMorph; }
    void SetBodyMorph(const BodyMorphStats& morph) noexcept { m_bodyMorph = morph; }

    [[nodiscard]] const Vector3D& GetPosition() const noexcept { return m_position; }
    void SetPosition(const Vector3D& pos) noexcept { m_position = pos; }
    [[nodiscard]] float GetRotation() const noexcept { return m_rotationY; }
    void SetRotation(float angle) noexcept { m_rotationY = angle; }

    [[nodiscard]] uint64_t GetUID() const noexcept { return m_uid; }
    [[nodiscard]] const std::string& GetNickname() const noexcept { return m_nickname; }
    [[nodiscard]] const std::vector<Item>& GetInventory() const noexcept { return m_inventory; }
    [[nodiscard]] SurvivalStats& GetStats() noexcept { return m_stats; }

    // Прототипы методов управления, перемещения и расчетов высот/веса
    void UpdateMovementState(float deltaTime, bool isSprinting, bool isCtrlPressed);
    void Move(const Vector3D& direction, float speed, float deltaTime);
    
    [[nodiscard]] bool IsMoving() const noexcept;
    [[nodiscard]] float GetCurrentMapTileHeight() const noexcept;
    [[nodiscard]] float GetEquippedArmorWeight() const noexcept;
    
    // Шлюз связи с кузнечным верстаком CraftingManager
    void AddItemToInventory(uint32_t itemId, uint32_t count);

    // Логика инвентаря, выживания и применения медикаментов
    bool AddItem(uint32_t itemId, uint16_t qty, float durability = 1.0f);
    bool RemoveItem(size_t slotIndex, uint16_t qty);
    bool UseItem(size_t slotIndex);
    bool EquipItem(size_t slotIndex);
    void UpdateSurvival(float deltaTime);

    // Сериализация стейта для сетевого P2P-кооператива
    std::vector<uint8_t> SerializeState() const;
    bool DeserializeState(const std::vector<uint8_t>& buffer, size_t& offset);
};

} // namespace Centralia
