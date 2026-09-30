#pragma once
#include "core/Math3D.hpp"
#include <string>
#include <vector>
#include <cstdint>

namespace Centralia {

enum class CharacterGender : uint8_t {
    Male,
    Female
};

#pragma pack(push, 1)
struct BodyMorphStats {
    CharacterGender gender;
    float breastSize;        
    float intimateInt;       
    float gluteusSize;       
    float heightScale;       
    float muscleMass;        
};
#pragma pack(pop)

enum class ItemType : uint8_t {
    Weapon     = 0,
    Armor      = 1,
    Medical    = 2,
    Resource   = 3,
    Consumable = 4
};

struct Item {
    uint32_t id;          
    uint16_t quantity;    
    float durability;     
};  

struct SurvivalStats {
    float health    = 100.0f; 
    float hunger    = 100.0f; 
    float thirst    = 100.0f; 
    float radiation = 0.0f;   
};

class Player {
private:
    uint64_t m_uid;                 
    std::string m_nickname;         
    SurvivalStats m_stats;          
    std::vector<Item> m_inventory;  
    size_t m_maxInventorySlots;     
    uint32_t m_level = 1;                     // Уровень игрока
    float m_memoryAnchorScale = 100.0f;       // Неактивная шкала — якорь памяти для защиты от смещения миров

    uint32_t m_activeWeaponId;
    uint32_t m_activeArmorId;


    Vector3D m_position;
    float m_rotationY;              
    BodyMorphStats m_bodyMorph;     

public:
    Player();
    Player(uint64_t uid, const std::string& name, size_t slots = 20);
    ~Player();

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
    
    // Управление уровнем
    [[nodiscard]] uint32_t GetPlayerLevel() const noexcept { return m_level; }
    void SetPlayerLevel(uint32_t level) noexcept { m_level = level; }
    void AddPlayerLevel(uint32_t amount = 1) noexcept { m_level += amount; }

    // Управление шкалой «якоря памяти» (смещение миров)
    [[nodiscard]] float GetMemoryAnchorScale() const noexcept { return m_memoryAnchorScale; }
    void SetMemoryAnchorScale(float scale) noexcept { m_memoryAnchorScale = scale; }

    [[nodiscard]] const std::vector<Item>& GetInventory() const noexcept { return m_inventory; }
    [[nodiscard]] SurvivalStats& GetStats() noexcept { return m_stats; }

    void UpdateMovementState(float deltaTime, bool isSprinting, bool isCtrlPressed);
    void Move(const Vector3D& direction, float speed, float deltaTime);
    
    [[nodiscard]] bool IsMoving() const noexcept;
    [[nodiscard]] float GetCurrentMapTileHeight() const noexcept;
    [[nodiscard]] float GetEquippedArmorWeight() const noexcept;
    
    void AddItemToInventory(uint32_t itemId, uint32_t count);

    bool AddItem(uint32_t itemId, uint16_t qty, float durability = 1.0f);
    bool RemoveItem(size_t slotIndex, uint16_t qty);
    bool UseItem(size_t slotIndex);
    bool EquipItem(size_t slotIndex);
    void UpdateSurvival(float deltaTime);

    std::vector<uint8_t> SerializeState() const;
    bool DeserializeState(const std::vector<uint8_t>& buffer, size_t& offset);
};

} // namespace Centralia