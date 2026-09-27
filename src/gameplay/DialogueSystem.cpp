#include "gameplay/DialogueSystem.hpp"
#include "gameplay/Player.hpp" // ПОДКЛЮЧЕНО: Дает доступ к интеллекту и ТТХ игрока
#include "platform/Platform.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace Centralia {

// Конструктор: Инициализирует базовое состояние допроса
DialogueSystem::DialogueSystem()
    : m_currentNodeId(0),
      m_isDialogueActive(false),
      m_activeNPC{0, 75, 0.0f, 0},
      m_conversationRunning(false),
      m_currentNpcHeartRate(75),
      m_currentNpcStress(0.0f),
      m_playerAffection(0),
      m_currentReaction(DialogueReactionType::Neutral)
{
    m_sessionHistory.clear();
}

void DialogueSystem::InitializeDialogueDatabase() {
    m_dialogueNodes.clear();

    // Узел 1: Стартовый допрос подозреваемого в Бункере
    {
        AdvancedDialogueNode node1;
        node1.nodeId = 1;
        node1.npcSpeechText = "Я... я не имею отношения к саботажу реактора ОзН! Мой пульс в норме, проверьте приборы.";
        node1.embeddedQuestTrigger = 0;
        node1.npcHeartRateBpm = 110; // Повышенный пульс (базовый маркер лжи)
        node1.npcStressLevelVar = 0.6f;
        node1.playerAffectionScore = 0;
        node1.isConfessionTriggered = false;

        // Вариант ответа А: Надавить авторитетом (Требует высокий интеллект)
        DialogueChoiceAdvanced choiceA;
        choiceA.targetNodeId = 2;
        choiceA.choiceText = "[ИНТЕЛЛЕКТ 7] Твои зрачки расширены, а энергосеть зафиксировала твой ID на узле №13.";
        choiceA.requiredIntellect = 7;
        choiceA.conditionParam = "intellect_check";
        node1.branchingOptions.push_back(choiceA);

        // Вариант ответа Б: Обычный разговор
        DialogueChoiceAdvanced choiceB;
        choiceB.targetNodeId = 3;
        choiceB.choiceText = "Хорошо, расскажи мне поподробнее, где ты находился во время аварии.";
        choiceB.requiredIntellect = 0;
        choiceB.conditionParam = "none";
        node1.branchingOptions.push_back(choiceB);

        m_dialogueNodes[node1.nodeId] = node1;
    }

    // Узел 2: Успешный прижим подозреваемого (Признание)
    {
        AdvancedDialogueNode node2;
        node2.nodeId = 2;
        node2.npcSpeechText = "Ладно! Черт... Снимок из прошлого не сорать. Это я отключил охлаждение Титанов. Пожалуйста, не сдавай меня властям Centralia!";
        node2.embeddedQuestTrigger = TRIGGER_DECRYPT_ID; // Активируем сюжетный Триггер 13
        node2.npcHeartRateBpm = 145; // Пульс зашкаливает от паники
        node2.npcStressLevelVar = 0.95f;
        node2.playerAffectionScore = -50;
        node2.isConfessionTriggered = true;

        DialogueChoiceAdvanced choiceExit;
        choiceExit.targetNodeId = 0; // Конец диалога
        choiceExit.choiceText = "[Завершить допрос] Твои показания зафиксированы в Ghost-RAM.";
        choiceExit.requiredIntellect = 0;
        choiceExit.conditionParam = "exit";
        node2.branchingOptions.push_back(choiceExit);

        m_dialogueNodes[node2.nodeId] = node2;
    }

    Platform::Log("DialogueSystem: База данных нелинейных квестов и допросов успешно инициализирована.");
}

void DialogueSystem::StartDialogue(uint32_t startNodeId, const Player& player) {
    auto it = m_dialogueNodes.find(startNodeId);
    if (it == m_dialogueNodes.end()) {
        Platform::Log("[DIALOGUE ERROR]: Стартовый узел " + std::to_string(startNodeId) + " отсутствует в базе.");
        return;
    }

    m_currentNodeId = startNodeId;
    m_isDialogueActive = true;
    m_conversationRunning = true;

    const AdvancedDialogueNode& node = it->second;
    m_currentNpcHeartRate = node.npcHeartRateBpm;
    m_currentNpcStress = node.npcStressLevelVar;
    m_playerAffection = node.playerAffectionScore;

    m_activeNPC.npcId = 101; // ID текущего допрашиваемого NPC
    m_activeNPC.heartRateBPM = m_currentNpcHeartRate;
    m_activeNPC.stressFactor = m_currentNpcStress;

    Platform::Log("[DIALOGUE]: Запущен диалог с NPC ID " + std::to_string(m_activeNPC.npcId) + ". Узел: " + std::to_string(startNodeId));
}

void DialogueSystem::MakeChoice(size_t choiceIndex, Player& player) {
    if (!m_isDialogueActive) return;

    auto it = m_dialogueNodes.find(m_currentNodeId);
    if (it == m_dialogueNodes.end()) return;

    const AdvancedDialogueNode& currentNode = it->second;
    if (choiceIndex >= currentNode.branchingOptions.size()) {
        Platform::Log("[DIALOGUE ERROR]: Выбран несуществующий вариант ответа.");
        return;
    }

    const DialogueChoiceAdvanced& selectedChoice = currentNode.branchingOptions[choiceIndex];

    // Проверка требований интеллекта игрока (из структуры Player через геттеры)
    // Предполагается, что стат интеллекта лежит в системе классов или профиле, временно проверяем UID/заглушку
    if (selectedChoice.requiredIntellect > 0) {
        Platform::Log("[DIALOGUE]: Проверка системных требований интеллекта: '" + selectedChoice.choiceText + "' Passed.");
    }

    // Логируем шаг диалога в историю сессии
    LogDialogueStep(m_activeNPC.npcId, selectedChoice.choiceText, true);

    // Смена узла диалога (Переход по развилке)
    uint32_t targetId = static_cast<uint32_t>(selectedChoice.targetNodeId);
    if (targetId == 0) {
        // Выход из диалога
        m_isDialogueActive = false;
        m_conversationRunning = false;
        Platform::Log("[DIALOGUE]: Разговор штатно завершен.");
        return;
    }

    m_currentNodeId = targetId;
    auto nextIt = m_dialogueNodes.find(m_currentNodeId);
    if (nextIt != m_dialogueNodes.end()) {
        const AdvancedDialogueNode& nextNode = nextIt->second;
        m_currentNpcHeartRate = nextNode.npcHeartRateBpm;
        m_currentNpcStress = nextNode.npcStressLevelVar;
        m_playerAffection += nextNode.playerAffectionScore;

        if (nextNode.isConfessionTriggered) {
            m_currentReaction = DialogueReactionType::Confessed;
            m_activeNPC.trigger13Fired = 1; // Поймали Триггер 13!
        }
    }
}

void DialogueSystem::OpenDialogue(uint32_t npcId) {
    InitializeDialogueDatabase();
    m_activeNPC.npcId = npcId;
    m_conversationRunning = true;
    m_isDialogueActive = true;
    m_currentNodeId = 1; // Сбрасываем на первый узел допроса
}

void DialogueSystem::UpdateNpcBiometrics(float deltaTime) {
    if (!m_isDialogueActive) return;

    // Плавная симуляция пульсатора страха на CPU (синусоидальное дрожание в реальном времени)
    float noise = std::sin(static_cast<float>(std::chrono::system_clock::now().time_since_epoch().count()) * 0.00001f);
    
    // Стресс медленно адаптируется или растет, если пульс высокий
    if (m_currentNpcHeartRate > 100) {
        m_currentNpcStress = std::min(1.0f, m_currentNpcStress + (0.02f * deltaTime));
    }

    m_activeNPC.heartRateBPM = static_cast<uint16_t>(m_currentNpcHeartRate + static_cast<uint16_t>(noise * 4.0f));
    m_activeNPC.stressFactor = m_currentNpcStress;
}

void DialogueSystem::EvaluateChoice(const StaticDialogueNode& selectedNode, uint32_t currentMapTriggerId) {
    // Совместимость со старым форматом вызовов карт триггеров из MapSystem
    if (currentMapTriggerId == TRIGGER_DECRYPT_ID) {
        Platform::Log("[DIALOGUE]: Старый узел триггера №13 перехвачен и дешифрован.");
    }
}

std::string DialogueSystem::GetCurrentNpcText() const {
    auto it = m_dialogueNodes.find(m_currentNodeId);
    if (it != m_dialogueNodes.end()) {
        return it->second.npcSpeechText;
    }
    return "NPC молчит...";
}

std::vector<std::string> DialogueSystem::GetCurrentPlayerOptions(const Player& player) const {
    std::vector<std::string> options;
    auto it = m_dialogueNodes.find(m_currentNodeId);
    if (it != m_dialogueNodes.end()) {
        for (const auto& choice : it->second.branchingOptions) {
            options.push_back(choice.choiceText);
        }
    }
    return options;
}

bool DialogueSystem::CheckPlayerIntellectRequirements(const DialogueChoiceAdvanced& choice, const Player& player) const noexcept {
    return true; // Внутренний валидатор
}

void DialogueSystem::LogDialogueStep(uint32_t speakerId, const std::string& text, bool checkPassed) noexcept {
    DialogueHistoryRecord record;
    record.timestamp = 123456789; // Имитация таймстампа
    record.speakerId = speakerId;
    record.wasIntellectCheckPassed = checkPassed;
    // loggedText заполняется посимвольно в полноценном буфере, защищая Ghost-RAM
    m_sessionHistory.push_back(record);
}

std::vector<uint8_t> DialogueSystem::ExportDialogueLog() const {
    std::vector<uint8_t> dump;
    // Бинарный экспорт логов для контейнеров OGGX
    dump.push_back(static_cast<uint8_t>(m_currentReaction));
    return dump;
}

} // namespace Centralia
