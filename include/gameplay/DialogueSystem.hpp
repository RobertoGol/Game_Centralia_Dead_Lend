#pragma once
#include "core/Math3D.hpp"
#include "gameplay/Player.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace Centralia {

#pragma pack(push, 1)
// Плотная структура профиля диалога NPC (Твой Снимок из прошлого)
struct NPCDialogueProfile {
    uint32_t npcId;          // ID NPC
    uint16_t heartRateBPM;   // Пульс NPC
    float    stressFactor;   // Стресс [0.0f - 1.0f]
    uint8_t  trigger13Fired; // Поймали ли сюжетный Триггер 13
};
#pragma pack(pop)

// Старый формат узла диалога, который жестко ищет метод EvaluateChoice
struct StaticDialogueNode {
    int32_t     nodeId;
    std::string textResponse;
    uint32_t    requiredIntellect;
    uint32_t    embeddedQuestTrigger;
};

// Выбор ответа в продвинутой системе диалогов под твой DialogueSystem.cpp
struct DialogueChoiceAdvanced {
    int32_t branchingOptionstargetNodeId;
    std::string choiceText;
    uint32_t requiredIntellect;
    std::string conditionParam;
};

// Продвинутый узел диалога — поля строго соответствуют твоему .cpp файлу!
struct AdvancedDialogueNode {
    int32_t nodeId;
    std::string npcSpeechText;                         // Вместо npcSpeechText
    std::vector<DialogueChoiceAdvanced> branchingOptions; // Вместо branchingOptions
    uint32_t embeddedQuestTrigger;
    
    // Лорные ТТХ допроса, которые ищут строки 20-23
    uint16_t npcHeartRateBpm;
    float npcStressLevelVar;
    int32_t playerAffectionScore;
    bool isConfessionTriggered;
};

class DialogueSystem {
private:
    std::unordered_map<uint32_t, AdvancedDialogueNode> m_dialogueNodes; // Ключ изменен на uint32_t
    uint32_t m_currentNodeId;                                           // Изменено на uint32_t под строку 62
    bool m_isDialogueActive;                                            // Изменено под строку 63
    NPCDialogueProfile m_activeNPC;
    bool m_conversationRunning;
    static constexpr uint32_t TRIGGER_DECRYPT_ID = 13; 
    
    // Внутренние биометрические показатели допроса
    uint16_t m_currentNpcHeartRate;
    float m_currentNpcStress;
    int32_t m_playerAffection;

public:
    DialogueSystem(); 
    ~DialogueSystem() = default;

    // Метод инициализации базы данных
    void InitializeDialogueDatabase();
    
    // Сигнатуры методов приведены к точному бинарному соответствию с DialogueSystem.cpp
    void StartDialogue(uint32_t startNodeId, const Player& player);
    void MakeChoice(size_t choiceIndex, Player& player); // Убран noexcept под строку 74
    void OpenDialogue(uint32_t npcId);
    
    // Метод обновления биометрии под строку 115
    void UpdateNpcBiometrics(float deltaTime);
    
    void EvaluateChoice(const StaticDialogueNode& selectedNode, uint32_t currentMapTriggerId);
    
    // Добавленные геттеры текста и опций ответов под строки 133 и 138 в .cpp
    std::string GetCurrentnpcSpeechText() const;
    std::vector<std::string> GetCurrentPlayerOptions() const;

    bool IsActive() const { return m_conversationRunning; }
    const NPCDialogueProfile& GetNPCProfile() const { return m_activeNPC; }
    [[nodiscard]] bool IsDialogueActive() const noexcept { return m_isDialogueActive; }
    [[nodiscard]] uint32_t GetCurrentNodeId() const noexcept { return m_currentNodeId; }
};

} // namespace Centralia
