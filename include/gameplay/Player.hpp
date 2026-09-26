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
    
    // --- ИНТЕГРАЦИЯ РЕДАКТОРА ПЕРСОНАЖА ---
    BodyMorphStats m_bodyMorph; // Хранит точные слайдеры анатомии для GPU-шейдера

public:
    Player() : m_uid(777), m_nickname("Vault_Survivor"), m_maxInventorySlots(20), 
               m_activeWeaponId(0), m_activeArmorId(0), m_position(0.0f,0.0f,0.0f) {

        // Дефолтные параметры анатомии, если редактор пропущен
        m_bodyMorph.gender = CharacterGender::Male;
        m_bodyMorph.breastSize = 1.0f;
        m_bodyMorph.intimateInt = 1.0f;
        m_bodyMorph.gluteusSize = 1.0f;
        m_bodyMorph.heightScale = 1.0f;
        m_bodyMorph.muscleMass = 1.0f;
    }

    // Геттер для передачи данных морфинга в Renderer3D.cpp для деформации вершин
    [[nodiscard]] const BodyMorphStats& GetBodyMorph() const noexcept { return m_bodyMorph; }
    void SetBodyMorph(const BodyMorphStats& morph) noexcept { m_bodyMorph = morph; }

    // Конструктор с параметрами, который уже есть
    Player(uint64_t uid, const std::string& name, size_t slots = 20);
    ~Player();

    // 2. Метод обновления стейта движения из main.cpp:59
    const Vector3D& GetPosition() const { return m_position; }
    void SetPosition(const Vector3D& pos) { m_position = pos; }
    
    float GetRotation() const { return m_rotationY; }
    void SetRotation(float angle) { m_rotationY = angle; }

    // 2. Метод обновления стейта движения из main.cpp:59
    void UpdateMovementState(float deltaTime, bool isSprinting, bool isCtrlPressed) {
        // Логика переключения скоростей и траты выносливости
    }

    void Move(const Vector3D& direction, float speed, float deltaTime) {
        m_position = m_position + (direction.Normalize() * speed * deltaTime);
    }
    
    // 3. Метод проверки движения персонажа из main.cpp:66
    [[nodiscard]] bool IsMoving() const noexcept {
        return true; // Возвращаем true, если вектор скорости не нулевой
    }

    // 4. Метод получения высоты слоя карты из main.cpp:78
    [[nodiscard]] float GetCurrentMapTileHeight() const noexcept {
        return 51.0f; // Возвращаем базовый высотный слой земли из test.map
    }
    
    // Добавляем строго сюда для связи с CraftingManager:
    void AddItemToInventory(uint32_t itemId, uint32_t count) {
        AddItem(itemId, static_cast<uint16_t>(count));
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
