#pragma once
#include "core/Math3D.hpp"
#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <unordered_map>

namespace Centralia {

// Опережающее объявление игрока защищает текстовый движок от циклических зависимостей
class Player;

// Категории реакций NPC на допросе (Для нелинейных квестов и триггеров)
enum class DialogueReactionType : uint8_t {
    Neutral     = 0,
    Aggressive  = 1,
    Terrified   = 2,
    Friendly    = 3,
    Confessed   = 4
};

#pragma pack(push, 1)
// Плотная структура профиля диалога NPC (Снимок из прошлого для сохранений)
struct NPCDialogueProfile {
    uint32_t npcId;          // ID NPC
    uint16_t heartRateBPM;   // Пульс NPC
    float    stressFactor;   // Стресс [0.0f - 1.0f]
    uint8_t  trigger13Fired; // Поймали ли сюжетный Триггер 13
};
#pragma pack(pop)

// Стандартный формат узла диалога, который жестко ищет метод EvaluateChoice
struct StaticDialogueNode {
    int32_t     nodeId;
    std::string textResponse;
    uint32_t    requiredIntellect;
    uint32_t    embeddedQuestTrigger;
};

// Выбор ответа в продвинутой системе диалогов под DialogueSystem.cpp
struct DialogueChoiceAdvanced {
    int32_t     targetNodeId;         // Синхронизировано со строкой 98 в .cpp
    std::string choiceText;
    uint32_t    requiredIntellect;
    std::string conditionParam;
};

// Продвинутый узел диалога — поля строго соответствуют твоему .cpp файлу!
struct AdvancedDialogueNode {
    uint32_t nodeId;                                      // Синхронизировано со строкой 62 в .cpp
    std::string npcSpeechText;                            // Строка 29 в .cpp
    std::vector<DialogueChoiceAdvanced> branchingOptions; // Строки 17-18 в .cpp
    uint32_t embeddedQuestTrigger;
    
    // Лорные ТТХ допроса, которые ищут строки 20-23 и 103-106 в .cpp
    uint16_t npcHeartRateBpm;
    float npcStressLevelVar;
    int32_t playerAffectionScore;
    bool isConfessionTriggered;
};

// Дополнительная структура для квестовых логов текстового движка Centralia
struct DialogueHistoryRecord {
    uint64_t  timestamp;
    uint32_t  speakerId;
    char      loggedText[128];
    bool      wasIntellectCheckPassed;
};

class DialogueSystem {
private:
    std::unordered_map<uint32_t, AdvancedDialogueNode> m_dialogueNodes; // Ключ uint32_t под .cpp
    uint32_t m_currentNodeId;                                           // Соответствует строке 62 в .cpp
    bool m_isDialogueActive;                                            // Соответствует строке 63 в .cpp
    NPCDialogueProfile m_activeNPC;
    bool m_conversationRunning;
    
    static constexpr uint32_t TRIGGER_DECRYPT_ID = 13; // Наш Триггер 13 для Снимка из прошлого
    
    // Внутренние биометрические показатели допроса
    uint16_t m_currentNpcHeartRate;
    float m_currentNpcStress;
    int32_t m_playerAffection;

    // Сетка истории текущей сессии диалога
    std::vector<DialogueHistoryRecord> m_sessionHistory;
    size_t m_maxHistoryLogSize = 100;
    DialogueReactionType m_currentReaction;

    // Внутренние методы валидации скрытых параметров игрока
    bool CheckPlayerIntellectRequirements(const DialogueChoiceAdvanced& choice, const Player& player) const  ;
    void LogDialogueStep(uint32_t speakerId, const std::string& text, bool checkPassed)  ;

public:
    // Исправленный чистый конструктор и деструктор для MSVC cl.exe
    DialogueSystem();
    ~DialogueSystem() = default;
    
    // Метод инициализации базы реплик, вызываемый на строке 7 в .cpp
    void InitializeDialogueDatabase();
    
    // Методы диалогового стейта — приведены к точному бинарному соответствию с твоим DialogueSystem.cpp
    void StartDialogue(uint32_t startNodeId, const Player& player); // Строка 59 в .cpp
    void MakeChoice(size_t choiceIndex, Player& player);            // Строка 74 в .cpp
    void OpenDialogue(uint32_t npcId);
    
    // Метод обновления биометрии под строку 115 в .cpp (принимает ровно 1 аргумент!)
    void UpdateNpcBiometrics(float deltaTime);
    
    void EvaluateChoice(const StaticDialogueNode& selectedNode, uint32_t currentMapTriggerId);
    
    // Добавленные геттеры текста и опций ответов под строки 133 и 138 в .cpp (с const Player&)
    std::string GetCurrentNpcText() const;
    std::vector<std::string> GetCurrentPlayerOptions(const Player& player) const;

    // Геттеры расширенной лорной аналитики
    [[nodiscard]] bool IsActive() const   { return m_conversationRunning; };
    [[nodiscard]] const NPCDialogueProfile& GetNPCProfile() const   { return m_activeNPC; };
    [[nodiscard]] bool IsDialogueActive() const   { return m_isDialogueActive; };
    [[nodiscard]] uint32_t GetCurrentNodeId() const   { return m_currentNodeId; };
    [[nodiscard]] DialogueReactionType GetCurrentReaction() const   { return m_currentReaction; };
    
    // Метод выгрузки бинарных логов диалога для сохранений OGGX
    std::vector<uint8_t> ExportDialogueLog() const;
};

} // namespace Centralia
