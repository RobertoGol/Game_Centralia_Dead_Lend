#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "platform/Platform.hpp"
#include "gameplay/Player.hpp"

namespace Centralia {

struct DialogueOption {
    uint32_t optionId;
    std::string text;
    uint32_t requiredIntellect;
    uint32_t nextNodeId;
};

struct DialogueNode {
    uint32_t nodeId;
    std::string speakerName;
    std::string dialogueText;
    std::vector<DialogueOption> options;
};

class DialogueSystem {
private:
    std::unordered_map<uint32_t, DialogueNode> m_nodes;
    uint32_t m_currentNodeId;

public:
    DialogueSystem();
    ~DialogueSystem() = default;

    DialogueSystem(const DialogueSystem&) = delete;
    DialogueSystem& operator=(const DialogueSystem&) = delete;

    void LoadDialogueTree(uint32_t treeId);
    bool SelectOption(uint32_t optionId, Player& player);
    
    [[nodiscard]] bool CheckPlayerIntellectRequirements(uint32_t requiredIntellect, const Player& player) const noexcept;
    void LogDialogueStep(const std::string& entryText) const noexcept;

    [[nodiscard]] const DialogueNode* GetCurrentNode() const noexcept;
};

} // namespace Centralia