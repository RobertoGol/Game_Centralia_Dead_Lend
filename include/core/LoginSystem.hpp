#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "gameplay/Player.hpp"
#include "gameplay/ClassSystem.hpp" // Импортируем готовые HumanClass и TitanClass отсюда

namespace Centralia {

enum class CharacterRace : uint8_t {
    Human,
    Elf,
    Dwarf,
    HalfAlv
};

#pragma pack(push, 1)
struct CharacterSaveSlot {
    uint8_t       slotId;
    uint32_t      slotIndex;          // Нужно для LoginSystem.cpp:12
    char          characterName[32];  // Фиксированный размер строки под имя
    CharacterRace selectedRace;      
    uint32_t      level;
    uint32_t      health;
    uint8_t       isOccupied;         // Нужно для LoginSystem.cpp:65
    uint64_t      lastSavedTimestamp; // Нужно для LoginSystem.cpp:67
};
#pragma pack(pop)

class LoginSystem {
private:
    std::vector<CharacterSaveSlot> m_slots; 
    uint8_t m_selectedSlotIndex;
    bool m_isOnlineMode;
    std::string m_authToken;

public:
    // Конструктор по умолчанию без тела, так как в .cpp он уже реализован на строке 9
    LoginSystem(); 
    ~LoginSystem() = default;

    // Убраны noexcept, чтобы сигнатуры функций идеально совпали со src/core/LoginSystem.cpp
    void LoadSaveSlots();
    bool TryOnlineLogin(const std::string& username, const std::string& password);
    void InitializeOfflineMode();
    
    // Синхронизировано с типами из ClassSystem.hpp под строку 61 в .cpp
    bool CreateCharacterInSlot(uint8_t slotId, const std::string& name, CharacterRace race, HumanClass hClass, TitanClass tClass);
    bool LoadCharacterFromSlot(uint8_t slotId, Player& player, ClassSystem& outClassSystem);
    
    [[nodiscard]] const std::vector<CharacterSaveSlot>& GetSaveSlots() const noexcept { return m_slots; }
};

} // namespace Centralia
