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
// 58-байтовая плотная структура ячейки персонажа для Ghost-RAM и диска
struct CharacterSaveSlot {
    uint8_t       slotId;               // 1 байт
    uint32_t      slotIndex;            // 4 байта
    char          characterName[32];    // 32 байта: Фиксированная длина строки под имя
    CharacterRace selectedRace;         // 1 байт
    uint32_t      level;                // 4 байта
    uint32_t      health;               // 4 байта
    uint8_t       isOccupied;           // 1 байт
    uint64_t      lastSavedTimestamp;   // 8 байт
}; // Итого: ровно 55 байт + 3 байта внутреннего паддинга структуры = 58 байт
#pragma pack(pop)

class LoginSystem {
private:
    std::vector<CharacterSaveSlot> m_slots; 
    uint8_t m_selectedSlotIndex;
    bool m_isOnlineMode;
    std::string m_authToken;

public:
    // Конструктор инициализирует базовые слоты
    LoginSystem(); 
    ~LoginSystem() = default;

    // Запрет копирования менеджера авторизации во избежание дублирования токенов в ОЗУ
    LoginSystem(const LoginSystem&) = delete;
    LoginSystem& operator=(const LoginSystem&) = delete;

    // ИСПРАВЛЕНО: Метод увязан с внутренней оффлайн-обработкой для предотвращения ошибки LNK2019
    void LoadSaveSlots();
    
    bool TryOnlineLogin(const std::string& username, const std::string& password);
    void InitializeOfflineMode();
    
    bool CreateCharacterInSlot(uint8_t slotId, const std::string& name, CharacterRace race, HumanClass hClass, TitanClass tClass);
    bool LoadCharacterFromSlot(uint8_t slotId, Player& player, ClassSystem& outClassSystem);
    
    [[nodiscard]] const std::vector<CharacterSaveSlot>& GetSaveSlots() const noexcept { return m_slots; }
    [[nodiscard]] bool IsOnlineMode() const noexcept { return m_isOnlineMode; }
};

} // namespace Centralia
