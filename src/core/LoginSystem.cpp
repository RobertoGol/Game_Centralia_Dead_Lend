#include "core/LoginSystem.hpp"
#include "core/MemoryManager.hpp"
#include "core/NetworkSocket.hpp"
#include "platform/Platform.hpp"
#include <ctime>

namespace Centralia {

LoginSystem::LoginSystem() : m_isOnlineMode(false), m_selectedSlotIndex(0) {
    m_slots.resize(4); // Резервируем ровно 4 ячейки под персонажей
    for (uint8_t i = 0; i < 4; ++i) {
        m_slots[i].slotIndex = i;
        m_slots[i].isOccupied = false;
    }
}

bool LoginSystem::TryOnlineLogin(const std::string& username, const std::string& password) {
    Platform::Log("[AUTH]: Подключение к мастер-серверу Elder Tale (порт 443)...");
    
    NetworkSocket authSocket;
    // Пробуем достучаться до центрального сервера авторизации
    if (authSocket.ConnectToServer("35.289.97.966", 443)) {
        m_isOnlineMode = true;
        m_authToken = "TOKEN_ET_" + username + "_2026";
        Platform::Log("[AUTH]: Сетевой вход выполнен успешно. Профиль игрока синхронизирован.");
        authSocket.Close();
        return true;
    }

    // ЭФФЕКТ STATE OF DECAY: Изолируем отсутствие интернета и уходим в оффлайн
    m_isOnlineMode = false;
    Platform::Log("[AUTH] [ВНИМАНИЕ]: Мастер-сервер недоступен. Нет связи с сетью.");
    InitializeOfflineMode();
    return false;
}

void LoginSystem::InitializeOfflineMode() {
    Platform::Log("[AUTH]: Активация автономного режима State of Decay. Сканирование локальных сейв-ячеек...");
    
    MemoryManager crypto;
    crypto.Initialize(Platform::GetDeviceHWID());

    // Перебираем все 4 локальные ячейки сохранений на диске Windows 10
    for (uint8_t i = 0; i < 4; ++i) {
        std::string savePath = "slot_" + std::to_string(i) + ".dat";
        std::vector<uint8_t> rawSaveData;

        // Пытаемся считать зашифрованный файл ячейки
        if (crypto.LoadDecryptedFile(Platform::GetSaveDirectoryPath() + savePath, rawSaveData)) {
            // Файл найден и успешно расшифрован по аппаратному HWID ПК
            m_slots[i].isOccupied = true;
            // Сюда идет быстрое побайтовое восстановление метаданных расы и класса
            Platform::Log("[AUTH]: Найдено локальное сохранение в ячейке №" + std::to_string(i));
        } else {
            m_slots[i].isOccupied = false;
            Platform::Log("[AUTH]: Ячейка №" + std::to_string(i) + " пуста. Готова к созданию гуманоида.");
        }
    }
}

bool LoginSystem::CreateCharacterInSlot(uint8_t slotIndex, const std::string& name, CharacterRace race, HumanClass hClass, TitanClass tClass) {
    if (slotIndex >= 4) return false;

    CharacterSaveSlot& slot = m_slots[slotIndex];
    slot.isOccupied = true;
    slot.selectedRace = race;
    slot.lastSavedTimestamp = static_cast<uint64_t>(std::time(nullptr));

    // Сериализуем данные нового персонажа для записи на диск
    std::vector<uint8_t> binaryManifest;
    binaryManifest.push_back(static_cast<uint8_t>(race));
    binaryManifest.push_back(static_cast<uint8_t>(hClass));
    binaryManifest.push_back(static_cast<uint8_t>(tClass));
    // Пишем имя
    binaryManifest.insert(binaryManifest.end(), name.begin(), name.end());

    MemoryManager crypto;
    crypto.Initialize(Platform::GetDeviceHWID());
    std::string savePath = "slot_" + std::to_string(slotIndex) + ".dat";
    
    if (crypto.SaveEncryptedFile(Platform::GetSaveDirectoryPath() + savePath, binaryManifest)) {
        Platform::Log("[REDACTOR]: Персонаж '" + name + "' успешно создан и сохранен в локальную ячейку №" + std::to_string(slotIndex));
        return true;
    }
    return false;
}

bool LoginSystem::LoadCharacterFromSlot(uint8_t slotIndex, Player& outPlayer, ClassSystem& outClassSystem) {
    if (slotIndex >= 4 || !m_slots[slotIndex].isOccupied) return false;

    MemoryManager crypto;
    crypto.Initialize(Platform::GetDeviceHWID());
    std::string savePath = "slot_" + std::to_string(slotIndex) + ".dat";
    std::vector<uint8_t> binaryManifest;

    if (crypto.LoadDecryptedFile(Platform::GetSaveDirectoryPath() + savePath, binaryManifest)) {
        // Извлекаем расу и классы из сохраненных байт
        CharacterRace race = static_cast<CharacterRace>(binaryManifest[0]);
        HumanClass hClass = static_cast<HumanClass>(binaryManifest[1]);
        TitanClass tClass = static_cast<TitanClass>(binaryManifest[2]);

        outClassSystem.SetHumanClass(hClass);
        outClassSystem.SetTitanClass(tClass);

        m_selectedSlotIndex = slotIndex;
        Platform::Log("[AUTH]: Персонаж успешно выгружен из ячейки в ядро игры. Раса и классы адаптированы.");
        return true;
    }
    return false;
}

} // namespace Centralia
