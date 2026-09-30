#include "gameplay/DialogueSystem.hpp"
#include "gameplay/ClassSystem.hpp"
#include "gameplay/Player.hpp"
#include "video/AssetParser.hpp"
#include "platform/Platform.hpp"
#include "core/MemoryManager.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cstring>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS, ENUMS & DATA STRUCTURES
// ============================================================================

namespace {
    constexpr uint32_t DIALOGUE_SAVE_MAGIC = 0x4449414C; // "DIAL"
    constexpr uint32_t DIALOGUE_SAVE_VERSION = 1;
    constexpr size_t MAX_DIALOGUE_OPTIONS = 6;
}

enum class ConditionType {
    None,
    HasItem,
    LackItem,
    SkillCheck,
    AttributeCheck,
    GlobalVarEquals,
    GlobalVarGreater,
    GlobalVarLess,
    QuestStateActive,
    QuestStateCompleted
};

enum class ActionType {
    None,
    GiveItem,
    TakeItem,
    AddExperience,
    ChangeGlobalVar,
    StartQuest,
    CompleteQuest,
    StartCombat,
    OpenTradeWindow,
    ApplyBuff,
    TriggerCutscene,
    PlayAnimation
};

struct DialogueCondition {
    ConditionType type;
    uint32_t targetId;  // ID предмета, навыка, переменной или квеста
    int32_t requiredValue;
};

struct DialogueAction {
    ActionType type;
    uint32_t targetId;
    int32_t actionValue;
    std::string stringParam; // Для названий анимаций или катсцен
};

struct DialogueOption {
    std::string text;
    std::string targetNodeId;
    std::vector<DialogueCondition> conditions;
    std::vector<DialogueAction> actions;
    bool isRead; // Отмечать серым цветом прочитанные ветки
};

struct DialogueNode {
    std::string id;
    std::string speakerName;
    std::string text;
    std::string voiceAudioPath;
    float cameraZoom;
    std::vector<DialogueOption> options;
    std::vector<DialogueAction> entryActions; // Действия при входе в ноду
};

struct DialogueTree {
    std::string treeId;
    std::unordered_map<std::string, DialogueNode> nodes;
    std::string entryNodeId;
};

// ============================================================================
// SECTION 2: SYSTEM INTERNAL STATE & GLOBAL VARIABLES MANAGER
// ============================================================================

struct DialogueSystemImpl {
    std::unordered_map<std::string, DialogueTree> loadedTrees;
    
    // Global World State (Хранит переменные для квестов и отношений с NPC)
    std::unordered_map<uint32_t, int32_t> globalIntVariables;
    std::unordered_map<uint32_t, bool> questStatesActive;
    std::unordered_map<uint32_t, bool> questStatesCompleted;
    std::unordered_map<std::string, bool> readOptionsHistory;

    // Active Conversation State
    bool isConversationActive;
    std::string activeTreeId;
    std::string currentNodeId;
    uint32_t activeNpcEntityId;
    
    // External references
    ClassSystem* classSystemRef;
    Player* playerRef;

    DialogueSystemImpl() : isConversationActive(false), activeNpcEntityId(0), classSystemRef(nullptr), playerRef(nullptr) {}
};

DialogueSystem* DialogueSystem::s_instance = nullptr;

DialogueSystem::DialogueSystem() : m_pImpl(new DialogueSystemImpl()) {
    if (s_instance) {
        Platform::Log("[DIALOGUE FATAL]: Двойная инициализация системы диалогов!");
        std::terminate();
    }
    s_instance = this;
    Platform::Log("[DIALOGUE SYSTEM]: Подсистема нелинейных диалогов инициализирована.");
}

DialogueSystem::~DialogueSystem() {
    m_pImpl->loadedTrees.clear();
    m_pImpl->globalIntVariables.clear();
    m_pImpl->readOptionsHistory.clear();
    delete m_pImpl;
    s_instance = nullptr;
    Platform::Log("[DIALOGUE SYSTEM]: Менеджер диалогов выгружен.");
}

DialogueSystem& DialogueSystem::GetInstance() {
    if (!s_instance) std::terminate();
    return *s_instance;
}

void DialogueSystem::BindDependencies(ClassSystem* classSys, Player* player) {
    m_pImpl->classSystemRef = classSys;
    m_pImpl->playerRef = player;
}

// ============================================================================
// SECTION 3: CUSTOM DIALOGUE SCRIPT PARSER
// ============================================================================
// Формат:
// [TREE: Mayor_Town]
// Entry=Node_Start
// 
// [NODE: Node_Start]
// Speaker=Мэр
// Text=Слушай, путник, у нас проблемы с рейдерами.
// Audio=voices/mayor_greet_01.ogg
// Option=Что я получу за помощь?|Node_Reward|REQ:CHR>=5
// Option=Я разберусь с ними.|Node_Accept
// Option=Это не мои проблемы. Прощай.|EXIT
// 
// [NODE: Node_Reward]
// Speaker=Мэр
// Text=Дам тебе 100 крышек и дробовик. По рукам?
// ACT=GiveItem 500 100

namespace ScriptParser {
    static inline void Trim(std::string& s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    }

    static std::vector<std::string> Split(const std::string& s, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    DialogueCondition ParseCondition(const std::string& condStr) {
        DialogueCondition cond;
        cond.type = ConditionType::None;
        cond.targetId = 0;
        cond.requiredValue = 0;

        auto parts = Split(condStr, ':');
        if (parts.size() < 2) return cond;
        
        std::string typeStr = parts[0];
        std::string expr = parts[1];
        Trim(typeStr); Trim(expr);

        if (typeStr == "REQ_SKILL" || typeStr == "REQ_ATTR") {
            auto opPos = expr.find(">=");
            if (opPos != std::string::npos) {
                cond.targetId = std::stoul(expr.substr(0, opPos));
                cond.requiredValue = std::stoi(expr.substr(opPos + 2));
                cond.type = (typeStr == "REQ_SKILL") ? ConditionType::SkillCheck : ConditionType::AttributeCheck;
            }
        } 
        else if (typeStr == "HAS_ITEM") {
            auto spacePos = expr.find(" ");
            if (spacePos != std::string::npos) {
                cond.targetId = std::stoul(expr.substr(0, spacePos));
                cond.requiredValue = std::stoi(expr.substr(spacePos + 1));
                cond.type = ConditionType::HasItem;
            }
        }
        else if (typeStr == "VAR_EQ") {
            auto spacePos = expr.find(" ");
            if (spacePos != std::string::npos) {
                cond.targetId = std::stoul(expr.substr(0, spacePos));
                cond.requiredValue = std::stoi(expr.substr(spacePos + 1));
                cond.type = ConditionType::GlobalVarEquals;
            }
        }
        return cond;
    }

    DialogueAction ParseAction(const std::string& actStr) {
        DialogueAction act;
        act.type = ActionType::None;
        act.targetId = 0;
        act.actionValue = 0;

        auto parts = Split(actStr, ' ');
        if (parts.empty()) return act;

        std::string cmd = parts[0];
        Trim(cmd);

        if (cmd == "GIVE_ITEM" && parts.size() >= 3) {
            act.type = ActionType::GiveItem;
            act.targetId = std::stoul(parts[1]);
            act.actionValue = std::stoi(parts[2]);
        } 
        else if (cmd == "START_COMBAT") {
            act.type = ActionType::StartCombat;
        } 
        else if (cmd == "SET_VAR" && parts.size() >= 3) {
            act.type = ActionType::ChangeGlobalVar;
            act.targetId = std::stoul(parts[1]);
            act.actionValue = std::stoi(parts[2]);
        }
        else if (cmd == "OPEN_TRADE") {
            act.type = ActionType::OpenTradeWindow;
        }
        else if (cmd == "START_QUEST" && parts.size() >= 2) {
            act.type = ActionType::StartQuest;
            act.targetId = std::stoul(parts[1]);
        }
        return act;
    }
}

bool DialogueSystem::LoadDialogueTree(const std::string& virtualPath) {
    std::vector<uint8_t> rawData = AssetParser::GetInstance().LoadRawDataSync(virtualPath);
    if (rawData.empty()) {
        Platform::Log("[DIALOGUE ERROR]: Файл скрипта не найден: " + virtualPath);
        return false;
    }

    std::string scriptContent(rawData.begin(), rawData.end());
    std::istringstream stream(scriptContent);
    std::string line;

    DialogueTree tree;
    DialogueNode* currentNode = nullptr;

    while (std::getline(stream, line)) {
        ScriptParser::Trim(line);
        if (line.empty() || line[0] == '#') continue; // Комментарии

        if (line.front() == '[' && line.back() == ']') {
            std::string header = line.substr(1, line.length() - 2);
            auto tokens = ScriptParser::Split(header, ':');
            if (tokens.size() == 2) {
                std::string blockType = tokens[0];
                std::string blockId = tokens[1];
                ScriptParser::Trim(blockType); ScriptParser::Trim(blockId);

                if (blockType == "TREE") {
                    tree.treeId = blockId;
                } else if (blockType == "NODE") {
                    DialogueNode newNode;
                    newNode.id = blockId;
                    newNode.cameraZoom = 1.0f;
                    tree.nodes[blockId] = newNode;
                    currentNode = &tree.nodes[blockId];
                }
            }
            continue;
        }

        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = line.substr(0, eqPos);
        std::string val = line.substr(eqPos + 1);
        ScriptParser::Trim(key); ScriptParser::Trim(val);

        if (key == "Entry" && currentNode == nullptr) {
            tree.entryNodeId = val;
        } else if (currentNode != nullptr) {
            if (key == "Speaker") currentNode->speakerName = val;
            else if (key == "Text") currentNode->text = val;
            else if (key == "Audio") currentNode->voiceAudioPath = val;
            else if (key == "Zoom") currentNode->cameraZoom = std::stof(val);
            else if (key == "Option") {
                // Parse: Text|TargetNode|Conditions|Actions
                auto optTokens = ScriptParser::Split(val, '|');
                if (optTokens.size() >= 2) {
                    DialogueOption opt;
                    opt.text = optTokens[0];
                    opt.targetNodeId = optTokens[1];
                    opt.isRead = false;

                    // Парсинг условий и действий, если они есть
                    for (size_t i = 2; i < optTokens.size(); ++i) {
                        std::string extra = optTokens[i];
                        if (extra.find("REQ_") == 0 || extra.find("HAS_") == 0 || extra.find("VAR_") == 0) {
                            opt.conditions.push_back(ScriptParser::ParseCondition(extra));
                        } else {
                            opt.actions.push_back(ScriptParser::ParseAction(extra));
                        }
                    }
                    currentNode->options.push_back(opt);
                }
            }
            else if (key == "ACT") {
                currentNode->entryActions.push_back(ScriptParser::ParseAction(val));
            }
        }
    }

    if (tree.treeId.empty() || tree.nodes.empty()) {
        Platform::Log("[DIALOGUE ERROR]: Ошибка парсинга скрипта: " + virtualPath);
        return false;
    }

    m_pImpl->loadedTrees[tree.treeId] = tree;
    Platform::Log("[DIALOGUE MANAGER]: Загружено дерево диалогов: " + tree.treeId + " (Нод: " + std::to_string(tree.nodes.size()) + ")");
    return true;
}

// ============================================================================
// SECTION 4: CONDITION EVALUATION ENGINE (SKILL CHECKS & INVENTORY)
// ============================================================================

bool DialogueSystem::EvaluateCondition(const DialogueCondition& cond) const {
    switch (cond.type) {
        case ConditionType::None: 
            return true;
            
        case ConditionType::HasItem:
            if (m_pImpl->playerRef) {
                return m_pImpl->playerRef->GetItemCount(cond.targetId) >= static_cast<uint32_t>(cond.requiredValue);
            }
            return false;

        case ConditionType::LackItem:
            if (m_pImpl->playerRef) {
                return m_pImpl->playerRef->GetItemCount(cond.targetId) < static_cast<uint32_t>(cond.requiredValue);
            }
            return false;

        case ConditionType::SkillCheck:
            if (m_pImpl->classSystemRef) {
                int32_t skillVal = m_pImpl->classSystemRef->GetTotalSkillValue(static_cast<CharacterSkill>(cond.targetId));
                return skillVal >= cond.requiredValue;
            }
            return false;

        case ConditionType::AttributeCheck:
            if (m_pImpl->classSystemRef) {
                int32_t attrVal = m_pImpl->classSystemRef->GetTotalAttributeValue(static_cast<CharacterAttribute>(cond.targetId));
                return attrVal >= cond.requiredValue;
            }
            return false;

        case ConditionType::GlobalVarEquals: {
            auto it = m_pImpl->globalIntVariables.find(cond.targetId);
            int32_t val = (it != m_pImpl->globalIntVariables.end()) ? it->second : 0;
            return val == cond.requiredValue;
        }

        case ConditionType::GlobalVarGreater: {
            auto it = m_pImpl->globalIntVariables.find(cond.targetId);
            int32_t val = (it != m_pImpl->globalIntVariables.end()) ? it->second : 0;
            return val >= cond.requiredValue;
        }

        case ConditionType::QuestStateActive: {
            auto it = m_pImpl->questStatesActive.find(cond.targetId);
            return it != m_pImpl->questStatesActive.end() && it->second;
        }

        case ConditionType::QuestStateCompleted: {
            auto it = m_pImpl->questStatesCompleted.find(cond.targetId);
            return it != m_pImpl->questStatesCompleted.end() && it->second;
        }

        default:
            return false;
    }
}

// ============================================================================
// SECTION 5: ACTION EXECUTION ENGINE (COMBAT, LOOT, VARS)
// ============================================================================

void DialogueSystem::ExecuteAction(const DialogueAction& act) {
    switch (act.type) {
        case ActionType::None:
            break;

        case ActionType::GiveItem:
            if (m_pImpl->playerRef) {
                m_pImpl->playerRef->AddItemToInventory(act.targetId, static_cast<uint16_t>(act.actionValue));
                Platform::Log("[DIALOGUE ACTION]: Игрок получил предмет ID " + std::to_string(act.targetId) + " (x" + std::to_string(act.actionValue) + ")");
            }
            break;

        case ActionType::TakeItem:
            if (m_pImpl->playerRef) {
                // Реализация удаления предмета
                Platform::Log("[DIALOGUE ACTION]: Из инвентаря изъят предмет ID " + std::to_string(act.targetId));
            }
            break;

        case ActionType::ChangeGlobalVar:
            m_pImpl->globalIntVariables[act.targetId] = act.actionValue;
            Platform::Log("[DIALOGUE STATE]: Глобальная переменная #" + std::to_string(act.targetId) + " установлена в " + std::to_string(act.actionValue));
            break;

        case ActionType::StartCombat:
            Platform::Log("[DIALOGUE ACTION]: NPC становится ВРАЖДЕБНЫМ! Инициация боя.");
            EndConversation();
            // Вызов события в ИИ-системе (MonsterAISystem / CreatureAI)
            break;

        case ActionType::OpenTradeWindow:
            Platform::Log("[DIALOGUE ACTION]: Открытие окна торговли с NPC.");
            EndConversation();
            // Engine::GetInstance().ChangeState(EngineState::TradingMenu);
            break;

        case ActionType::StartQuest:
            m_pImpl->questStatesActive[act.targetId] = true;
            Platform::Log("[DIALOGUE ACTION]: Начат новый квест! ID: " + std::to_string(act.targetId));
            break;

        case ActionType::CompleteQuest:
            m_pImpl->questStatesActive[act.targetId] = false;
            m_pImpl->questStatesCompleted[act.targetId] = true;
            Platform::Log("[DIALOGUE ACTION]: Квест ВЫПОЛНЕН! ID: " + std::to_string(act.targetId));
            break;

        default:
            break;
    }
}

// ============================================================================
// SECTION 6: CONVERSATION FLOW (START, NAVIGATE, END)
// ============================================================================

bool DialogueSystem::StartConversation(const std::string& treeId, uint32_t npcEntityId) {
    if (m_pImpl->isConversationActive) return false;

    auto it = m_pImpl->loadedTrees.find(treeId);
    if (it == m_pImpl->loadedTrees.end()) {
        Platform::Log("[DIALOGUE ERROR]: Дерево диалога не найдено: " + treeId);
        return false;
    }

    m_pImpl->activeTreeId = treeId;
    m_pImpl->currentNodeId = it->second.entryNodeId;
    m_pImpl->activeNpcEntityId = npcEntityId;
    m_pImpl->isConversationActive = true;

    Platform::Log("[DIALOGUE]: Начата беседа с NPC. Дерево: " + treeId);

    // Выполнение entry-действий первой ноды (например, разворот NPC лицом к игроку)
    ProcessCurrentNodeEntry();

    return true;
}

void DialogueSystem::ProcessCurrentNodeEntry() {
    const DialogueTree& tree = m_pImpl->loadedTrees[m_pImpl->activeTreeId];
    const DialogueNode& node = tree.nodes.at(m_pImpl->currentNodeId);

    // 1. Выполнение скриптовых событий
    for (const auto& action : node.entryActions) {
        ExecuteAction(action);
    }

    // 2. Воспроизведение Voice Over (Аудиофайла)
    if (!node.voiceAudioPath.empty()) {
        // AudioEngine::PlayVoice(node.voiceAudioPath, m_pImpl->activeNpcEntityId);
    }

    // 3. Управление камерой (Наезд на лицо NPC в стиле Fallout 4 / Starfield)
    // CameraSystem::SetDialogueFocus(m_pImpl->activeNpcEntityId, node.cameraZoom);
}

void DialogueSystem::EndConversation() {
    if (!m_pImpl->isConversationActive) return;

    m_pImpl->isConversationActive = false;
    m_pImpl->activeTreeId.clear();
    m_pImpl->currentNodeId.clear();
    m_pImpl->activeNpcEntityId = 0;

    Platform::Log("[DIALOGUE]: Беседа завершена. Возврат к геймплею.");
    // CameraSystem::ReleaseDialogueFocus();
}

const DialogueNode* DialogueSystem::GetCurrentNode() const {
    if (!m_pImpl->isConversationActive) return nullptr;

    const DialogueTree& tree = m_pImpl->loadedTrees.at(m_pImpl->activeTreeId);
    return &tree.nodes.at(m_pImpl->currentNodeId);
}

std::vector<DialogueOption> DialogueSystem::GetValidOptions() const {
    std::vector<DialogueOption> validOptions;
    if (!m_pImpl->isConversationActive) return validOptions;

    const DialogueTree& tree = m_pImpl->loadedTrees.at(m_pImpl->activeTreeId);
    const DialogueNode& node = tree.nodes.at(m_pImpl->currentNodeId);

    for (const auto& option : node.options) {
        bool allConditionsMet = true;
        for (const auto& cond : option.conditions) {
            if (!EvaluateCondition(cond)) {
                allConditionsMet = false;
                break;
            }
        }

        if (allConditionsMet) {
            DialogueOption opt = option;
            // Проверка истории: читалась ли уже эта реплика
            std::string historyKey = m_pImpl->activeTreeId + ":" + m_pImpl->currentNodeId + ":" + opt.targetNodeId;
            opt.isRead = m_pImpl->readOptionsHistory[historyKey];
            
            // Если текст опции содержит теги навыков (например, "[Красноречие] Я не хочу драться."), 
            // в UI мы сможем это распарсить и подсветить
            validOptions.push_back(opt);
        }
    }

    return validOptions;
}

void DialogueSystem::SelectOption(size_t index) {
    if (!m_pImpl->isConversationActive) return;

    std::vector<DialogueOption> options = GetValidOptions();
    if (index >= options.size()) return;

    const DialogueOption& selected = options[index];

    // Выполнение действий, привязанных к ответу игрока
    for (const auto& action : selected.actions) {
        ExecuteAction(action);
    }

    // Запись в историю прочитанных реплик
    std::string historyKey = m_pImpl->activeTreeId + ":" + m_pImpl->currentNodeId + ":" + selected.targetNodeId;
    m_pImpl->readOptionsHistory[historyKey] = true;

    // Переход
    if (selected.targetNodeId == "EXIT" || selected.targetNodeId.empty()) {
        EndConversation();
    } else {
        m_pImpl->currentNodeId = selected.targetNodeId;
        ProcessCurrentNodeEntry();
    }
}

bool DialogueSystem::IsConversationActive() const noexcept {
    return m_pImpl->isConversationActive;
}

// ============================================================================
// SECTION 7: WORLD STATE API (FOR QUEST SCRIPTS)
// ============================================================================

void DialogueSystem::SetGlobalVariable(uint32_t varId, int32_t value) {
    m_pImpl->globalIntVariables[varId] = value;
}

int32_t DialogueSystem::GetGlobalVariable(uint32_t varId) const {
    auto it = m_pImpl->globalIntVariables.find(varId);
    return (it != m_pImpl->globalIntVariables.end()) ? it->second : 0;
}

// ============================================================================
// SECTION 8: BINARY SERIALIZATION (SAVING WORLD STATE & MEMORY)
// ============================================================================

uint32_t DialogueSystem::CalculateChecksum(const std::vector<uint8_t>& buffer) const noexcept {
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : buffer) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

std::vector<uint8_t> DialogueSystem::SerializeToBinary() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(4096);

    const uint8_t* magicPtr = reinterpret_cast<const uint8_t*>(&DIALOGUE_SAVE_MAGIC);
    buffer.insert(buffer.end(), magicPtr, magicPtr + sizeof(uint32_t));

    const uint8_t* verPtr = reinterpret_cast<const uint8_t*>(&DIALOGUE_SAVE_VERSION);
    buffer.insert(buffer.end(), verPtr, verPtr + sizeof(uint32_t));

    // 1. Сохранение Глобальных Переменных
    uint32_t varCount = static_cast<uint32_t>(m_pImpl->globalIntVariables.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&varCount), reinterpret_cast<const uint8_t*>(&varCount) + sizeof(uint32_t));
    
    for (const auto& [id, val] : m_pImpl->globalIntVariables) {
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&id), reinterpret_cast<const uint8_t*>(&id) + sizeof(uint32_t));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&val), reinterpret_cast<const uint8_t*>(&val) + sizeof(int32_t));
    }

    // 2. Сохранение Состояний Квестов
    uint32_t activeQuestCount = static_cast<uint32_t>(m_pImpl->questStatesActive.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&activeQuestCount), reinterpret_cast<const uint8_t*>(&activeQuestCount) + sizeof(uint32_t));
    
    for (const auto& [id, state] : m_pImpl->questStatesActive) {
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&id), reinterpret_cast<const uint8_t*>(&id) + sizeof(uint32_t));
        uint8_t s = state ? 1 : 0;
        buffer.push_back(s);
    }

    uint32_t compQuestCount = static_cast<uint32_t>(m_pImpl->questStatesCompleted.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&compQuestCount), reinterpret_cast<const uint8_t*>(&compQuestCount) + sizeof(uint32_t));
    
    for (const auto& [id, state] : m_pImpl->questStatesCompleted) {
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&id), reinterpret_cast<const uint8_t*>(&id) + sizeof(uint32_t));
        uint8_t s = state ? 1 : 0;
        buffer.push_back(s);
    }

    // 3. Сохранение Истории Прочитанных Реплик (чтобы серые опции оставались серыми)
    uint32_t historyCount = static_cast<uint32_t>(m_pImpl->readOptionsHistory.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&historyCount), reinterpret_cast<const uint8_t*>(&historyCount) + sizeof(uint32_t));

    for (const auto& [key, isRead] : m_pImpl->readOptionsHistory) {
        if (!isRead) continue;
        uint32_t keyLen = static_cast<uint32_t>(key.length());
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&keyLen), reinterpret_cast<const uint8_t*>(&keyLen) + sizeof(uint32_t));
        buffer.insert(buffer.end(), key.begin(), key.end());
    }

    uint32_t checksum = CalculateChecksum(buffer);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&checksum), reinterpret_cast<const uint8_t*>(&checksum) + sizeof(uint32_t));

    Platform::Log("[DIALOGUE SERIALIZE]: Состояния квестов и память NPC упакованы (" + std::to_string(buffer.size()) + " байт).");
    return buffer;
}

bool DialogueSystem::DeserializeFromBinary(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(uint32_t) * 6) return false;

    size_t payloadSize = buffer.size() - sizeof(uint32_t);
    std::vector<uint8_t> payloadData(buffer.begin(), buffer.begin() + payloadSize);
    uint32_t expectedChecksum = CalculateChecksum(payloadData);

    uint32_t storedChecksum = 0;
    std::memcpy(&storedChecksum, buffer.data() + payloadSize, sizeof(uint32_t));

    if (expectedChecksum != storedChecksum) {
        Platform::Log("[DIALOGUE LOAD ERROR]: CRC32 не совпадает. Повреждение данных квестов.");
        return false;
    }

    size_t cursor = 0;
    uint32_t magic = 0;
    std::memcpy(&magic, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    if (magic != DIALOGUE_SAVE_MAGIC) return false;

    uint32_t version = 0;
    std::memcpy(&version, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

    // 1. Чтение Глобальных Переменных
    uint32_t varCount = 0;
    std::memcpy(&varCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    m_pImpl->globalIntVariables.clear();
    for (uint32_t i = 0; i < varCount; ++i) {
        uint32_t id; int32_t val;
        std::memcpy(&id, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        std::memcpy(&val, buffer.data() + cursor, sizeof(int32_t)); cursor += sizeof(int32_t);
        m_pImpl->globalIntVariables[id] = val;
    }

    // 2. Чтение Состояний Квестов
    uint32_t activeQuestCount = 0;
    std::memcpy(&activeQuestCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    m_pImpl->questStatesActive.clear();
    for (uint32_t i = 0; i < activeQuestCount; ++i) {
        uint32_t id; uint8_t s;
        std::memcpy(&id, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        s = buffer[cursor++];
        m_pImpl->questStatesActive[id] = (s != 0);
    }

    uint32_t compQuestCount = 0;
    std::memcpy(&compQuestCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    m_pImpl->questStatesCompleted.clear();
    for (uint32_t i = 0; i < compQuestCount; ++i) {
        uint32_t id; uint8_t s;
        std::memcpy(&id, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        s = buffer[cursor++];
        m_pImpl->questStatesCompleted[id] = (s != 0);
    }

    // 3. Чтение Истории
    uint32_t historyCount = 0;
    std::memcpy(&historyCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    m_pImpl->readOptionsHistory.clear();
    for (uint32_t i = 0; i < historyCount; ++i) {
        uint32_t keyLen;
        std::memcpy(&keyLen, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        std::string key(buffer.begin() + cursor, buffer.begin() + cursor + keyLen);
        cursor += keyLen;
        m_pImpl->readOptionsHistory[key] = true;
    }

    Platform::Log("[DIALOGUE DESERIALIZE]: Мировая история диалогов успешно восстановлена.");
    return true;
}

} // namespace Centralia