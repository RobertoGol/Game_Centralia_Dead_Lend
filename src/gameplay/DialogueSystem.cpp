#include "gameplay/DialogueSystem.hpp"
#include "platform/Platform.hpp"
#include <algorithm>

namespace Centralia {

void DialogueSystem::InitializeDialogueDatabase() {
    m_dialogueNodes.clear();

    // Узел 1: Стартовый диалог в Убежище Обучения (аналог Fallout 4)
    AdvancedDialogueNode node1;
    node1.nodeId = 1;
    node1.npcSpeechText = "Приветствую тебя в центральном секторе, Выживший. Системы жизнеобеспечения бункера стабильны. Куда планируешь выдвигаться?";
    
    DialogueChoiceAdvanced choice1_1 = { "Я ищу выход во Внешний Мир Centralia.", 2, 0, 5 };
    DialogueChoiceAdvanced choice1_2 = { "Мне нужно воспользоваться верстаком для модификации колес.", 3, 0, 0 };
    node1.branchingOptions.push_back(choice1_1);
    node1.branchingOptions.push_back(choice1_2);
    
    node1.npcHeartRateBpm = 75;
    node1.npcStressLevelVar = 10.0f;
    node1.playerAffectionScore = 10.0f;
    node1.isConfessionTriggered = false;
    m_dialogueNodes[node1.nodeId] = node1;

    // Узел 2: Скрытая ветка, завязанная на "Триггер 13" (Снимок из прошлого)
    AdvancedDialogueNode node2;
    node2.nodeId = 2;
    node2.npcSpeechText = "Внешний Мир? Там бушуют радиационные штормы. Без силовой брони и герметичного транспорта ты там погибнешь... Погоди, что это у тебя в руках?";
    
    // Сюда мы привяжем проверку наличия предмета 1002 в инвентаре
    DialogueChoiceAdvanced choice2_1 = { "[ТРИГГЕР 13] Показать старый Снимок из прошлого.", 4, 0, 50 }; // Требует квестовый предмет
    DialogueChoiceAdvanced choice2_2 = { "Ничего. Пожалуй, я останусь в бункере.", 1, 0, -5 };
    node2.branchingOptions.push_back(choice2_1);
    node2.branchingOptions.push_back(choice2_2);
    
    node2.npcHeartRateBpm = 90; // Пульс растет при упоминании Внешнего Мира
    node2.npcStressLevelVar = 35.0f;
    node2.playerAffectionScore = 15.0f;
    m_dialogueNodes[node2.nodeId] = node2;

    // Узел 4: Ветка признания и глубокого доверия (Confession Recorded из Логов Akemi)
    AdvancedDialogueNode node4;
    node4.nodeId = 4;
    node4.npcSpeechText = "О боже... Это же снимок центрального реактора до Изоляции! Откуда он у тебя? Значит, легенды о коде 'Ox-n init' правдивы... Тớ thật sự thích cậu. Я доверяю тебе, вот ключ от гермозатвора.";
    
    DialogueChoiceAdvanced choice4_1 = { "Принять ключ и открыть ворота в Пустошь.", 5, 0, 100 };
    node4.branchingOptions.push_back(choice4_1);
    
    node4.npcHeartRateBpm = 135; // Критический пульс (Волнение/Признание)
    node4.npcStressLevelVar = 75.0f;
    node4.playerAffectionScore = 100.0f; // Максимальное доверие
    node4.isConfessionTriggered = true;  // Фиксация стейта в логах
    m_dialogueNodes[node4.nodeId] = node4;

    Platform::Log("DialogueSystem: База реплик и биометрии NPC успешно инициализирована.");
}

void DialogueSystem::StartDialogue(uint32_t startNodeId, const Player& player) {
    auto it = m_dialogueNodes.find(startNodeId);
    if (it != m_dialogueNodes.end()) {
        m_currentNodeId = startNodeId;
        m_isDialogueActive = true;
        
        // Передаем стартовую биометрию NPC в процессор движка
        m_currentNpcHeartRate = it->second.npcHeartRateBpm;
        m_currentNpcStress = it->second.npcStressLevelVar;
        m_playerAffection = it->second.playerAffectionScore;
        
        Platform::Log("[DIALOGUE]: Диалог запущен. Собеседник: NPC_" + std::to_string(startNodeId));
    }
}

void DialogueSystem::MakeChoice(size_t choiceIndex, Player& player) {
    if (!m_isDialogueActive) return;

    auto it = m_dialogueNodes.find(m_currentNodeId);
    if (it == m_dialogueNodes.end() || choiceIndex >= it->second.branchingOptions.size()) return;

    const auto& choice = it->second.branchingOptions[choiceIndex];

    // Проверка скрытого "Триггера 13": если выбор требует Снимок из прошлого (Узел №2, Вариант 0)
    if (m_currentNodeId == 2 && choiceIndex == 0) {
        bool hasPhoto = false;
        for (const auto& item : player.GetInventory()) {
            if (item.id == 1002) { // Снимок из прошлого найден в инвентаре
                hasPhoto = true;
                break;
            }
        }
        if (!hasPhoto) {
            Platform::Log("[DIALOGUE]: Ошибка! Узел заблокирован. У вас нет Снимка из прошлого.");
            return; // Сюжетный триггер не пропущен
        }
    }

    // Переходим на следующий узел дерева диалогов
    m_currentNodeId = choice.targetNodeId;
    
    // Обновляем базовую биометрию под новый узел
    auto nextIt = m_dialogueNodes.find(m_currentNodeId);
    if (nextIt != m_dialogueNodes.end()) {
        m_currentNpcHeartRate = nextIt->second.npcHeartRateBpm;
        m_currentNpcStress = nextIt->second.npcStressLevelVar;
        
        if (nextIt->second.isConfessionTriggered) {
            Platform::Log("[DIALOGUE SYSTEMS]: Сработало глубокое признание. Данные зафиксированы в Ghost-RAM.");
        }
    } else {
        m_isDialogueActive = false; // Конец ветки диалога, закрываем окно
        Platform::Log("[DIALOGUE]: Конец разговора.");
    }
}

void DialogueSystem::UpdateNpcBiometrics(float deltaTime) {
    if (!m_isDialogueActive) return;

    // Эффект живого сердцебиения: проц генерирует микро-колебания пульса на CPU,
    // создавая реалистичное поведение стресса на датчиках Пип-боя
    static float bioTimer = 0.0f;
    bioTimer += deltaTime;
    
    if (bioTimer >= 0.5f) { // Каждые полсекунды вносим флуктуации
        bioTimer = 0.0f;
        // Пульс колеблется в районе базового значения узла (+-3 удара)
        m_currentNpcHeartRate += (rand() % 3) - 1;
        
        // Стресс плавно затухает, если игрок не выбирает агрессивные реплики
        m_currentNpcStress = std::max(0.0f, m_currentNpcStress - (0.1f * deltaTime));
    }
}

std::string DialogueSystem::GetCurrentNpcText() const {
    auto it = m_dialogueNodes.find(m_currentNodeId);
    return (it != m_dialogueNodes.end()) ? it->second.npcSpeechText : "";
}

std::vector<std::string> DialogueSystem::GetCurrentPlayerOptions(const Player& player) const {
    std::vector<std::string> options;
    auto it = m_dialogueNodes.find(m_currentNodeId);
    if (it != m_dialogueNodes.end()) {
        for (const auto& choice : it->second.branchingOptions) {
            options.push_back(choice.textOption);
        }
    }
    return options;
}

} // namespace Centralia
