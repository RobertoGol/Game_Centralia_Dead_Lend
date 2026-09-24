#include <cstdint>
#include <string>
#include <array>
#include "gameplay/Math3D.hpp"

namespace Centralia {

#pragma pack(push, 1)
struct NPCDialogueProfile {
    uint32_t npcId;
    uint16_t heartRateBPM;   // Пульс NPC
    float    stressFactor;   // Стресс [0.0f - 1.0f]
    uint8_t  trigger13Fired; // Поймали ли сюжетный Триггер 13
};
#pragma pack(pop)

struct StaticDialogueNode {
    int32_t     nodeId;
    std::string textResponse;
    uint32_t    requiredIntellect;
    uint32_t    embeddedQuestTrigger;
};

class DialogueSystem {
private:
    NPCDialogueProfile m_activeNPC;
    bool m_conversationRunning;
    
    static constexpr uint32_t TRIGGER_DECRYPT_ID = 13; // Наш Триггер 13 для Снимка из прошлого

public:
    DialogueSystem();
    ~DialogueSystem() = default;

    void OpenDialogue(uint32_t npcId);
    void UpdateHeartBeat(float deltaTime, bool useIntimidation);
    void EvaluateChoice(const StaticDialogueNode& selectedNode, uint32_t currentMapTriggerId);

    bool IsActive() const { return m_conversationRunning; }
    const NPCDialogueProfile& GetNPCProfile() const { return m_activeNPC; }
};

} // namespace Centralia
