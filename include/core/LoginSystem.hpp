#pragma once
#include "gameplay/ClassSystem.hpp"
#include "gameplay/ModificationSystem.hpp" // Берем CharacterRace и CharacterSaveSlot из спецификации
#include <string>
#include <vector>

namespace Centralia {

class LoginSystem {
private:
    bool m_isOnlineMode = false;
    std::string m_authToken = "";
    std::vector<CharacterSaveSlot> m_slots;
    uint8_t m_selectedSlotIndex = 0;

public:
    LoginSystem();
    ~LoginSystem() = default;

    // Попытка авторизации через мастер-сервер (как в Elder Tale)
    // Если нет интернета — функция безопасно возвращает false, активируя State of Decay режим
    bool TryOnlineLogin(const std::string& username, const std::string& password);

    // Запуск автономного режима: чтение ячеек сохранений локально с диска ПК
    void InitializeOfflineMode();

    // Создание нового персонажа-гуманоида в выбранной ячейке (редактор Fallout 76)
    bool CreateCharacterInSlot(uint8_t slotIndex, const std::string& name, CharacterRace race, HumanClass hClass, TitanClass tClass);

    // Загрузка существующего стейта персонажа из файла
    bool LoadCharacterFromSlot(uint8_t slotIndex, Player& outPlayer, ClassSystem& outClassSystem);

    bool IsOnline() const { return m_isOnlineMode; }
    uint8_t GetSelectedSlot() const { return m_selectedSlotIndex; }
    const std::vector<CharacterSaveSlot>& GetAvailableSlots() const { return m_slots; }
};

} // namespace Centralia
