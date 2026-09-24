#pragma once
#include "gameplay/ModificationSystem.hpp" // Импортируем структуры AdvancedDialogueNode и DialogueBranchChoice
#include "gameplay/Player.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace Centralia {

class DialogueSystem {
private:
    std::unordered_map<uint32_t, AdvancedDialogueNode> m_dialogueNodes;
    uint32_t m_currentNodeId = 0;
    bool m_isDialogueActive = false;

    // Ссылки на текущие биометрические показатели активного NPC
    uint16_t m_currentNpcHeartRate = 72;
    float m_currentNpcStress = 15.0f;
    float m_playerAffection = 0.0f;

public:
    DialogueSystem() = default;
    ~DialogueSystem() = default;

    // Инициализация базы реплик и лора (Бункер Обучения и Пустошь)
    void InitializeDialogueDatabase();

    // Запуск разговора с конкретным NPC
    void StartDialogue(uint32_t startNodeId, const Player& player);

    // Выбор варианта ответа (индекс от 0 до размера списка choices)
    void MakeChoice(size_t choiceIndex, Player& player);

    // Симуляция изменения пульса и стресса NPC в реальном времени на CPU
    void UpdateNpcBiometrics(float deltaTime);

    // Геттеры для вывода биометрии на экран Пип-боя
    bool IsDialogueActive() const { return m_isDialogueActive; }
    uint32_t GetCurrentNodeId() const { return m_currentNodeId; }
    std::string GetCurrentNpcText() const;
    std::vector<std::string> GetCurrentPlayerOptions(const Player& player) const;
    
    uint16_t GetNpcHeartRate() const { return m_currentNpcHeartRate; }
    float GetNpcStress() const { return m_currentNpcStress; }
};

} // namespace Centralia
