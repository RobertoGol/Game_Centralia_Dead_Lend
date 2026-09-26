#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "core/Math3D.hpp" // Твой Vector3D
#include "gameplay/Player.hpp"  // Твой класс игрока

namespace Centralia {

// Архетипы ИИ для разделения скриптов поведения
enum class AIArchetype : uint8_t {
    Individual  = 0, // Обычные существа (Бегемоты, радиоактивные гули)
    SwarmDrone  = 1  // Роботы-дроны роя (улей)
};

// Стадии ментального и тактического состояния псевдо-ИИ
enum class BehaviorState : uint8_t {
    Patrolling     = 0, // Спокойный обход точек
    Investigating  = 1, // Медленное расследование точки падения гильзы
    SearchingArea  = 2, // Обыск углов и секторов в точке прибытия
    CombatChasing  = 3, // Прямой штурм / Атака в лоб
    FlankingTarget = 4  // Вторичный режим: обход с фланга (редкий сценарий для роботов роя)
};

struct AIIntelligenceProfile {
    uint32_t      monsterId;
    std::string   monsterLoreName;
    AIArchetype   archetype = AIArchetype::Individual;
    
    Vector3D      currentPosition;
    Vector3D      patrolTarget;
    Vector3D      basePatrolOrigin;     // Исходная центральная точка патруля для возврата
    Vector3D      lastKnownNoiseSource; // Фантомная точка интереса (куда упала гильза)
    
    BehaviorState currentBehavior = BehaviorState::Patrolling;
    
    float         searchRadiusTimer = 0.0f;
    float         suspicionLevel = 0.0f;
    float         flankAngle = 0.0f;
    
    // ТТХ Системы Повреждений (Полностью возвращены)
    float         health = 500.0f;
    float         maxHealth = 500.0f;
    float         baseHearingRadius = 15.0f;
    uint8_t       areLegsCrippled = 0;   // Повреждение ходовой части
    uint8_t       isEnraged = 0;         // Состояние ярости Бегемота (Enrage State)
};

class MonsterAISystem {
private:
    std::vector<AIIntelligenceProfile> m_simulatedAgents;

    // Константы акустики
    static constexpr float RAW_BASE_STEP_VOLUME = 6.0f;
    static constexpr float SPRINT_HEAVY_NOISE   = 18.0f;
    static constexpr float GHOST_SNEAK_DEBUFF   = 0.35f; // Пассивное снижение звукового загрязнения в приседе
    
    // Дистанции эшелонирования сети роботов роя при броске гильзы
    static constexpr float SWARM_LINK_RADIUS             = 25.0f;
    static constexpr float SWARM_IMMEDIATE_ATTACK_RADIUS = 12.0f; // Ближайшие (сразу штурмуют точку)
    static constexpr float SWARM_MID_DIST_SECTOR_RADIUS  = 35.0f; // Средние и дальние (сужают патруль)

public:
    MonsterAISystem() = default;
    ~MonsterAISystem() = default;

    /**
     * @brief Высокоуровневая обработка скриптов поведения псевдо-ИИ.
     */
    void ProcessAIScriptsTick(float deltaTime, const Player& player, bool isPlayerSprinting, bool isPlayerInGhostSneak);

    /**
     * @brief Механика броска гильзы игроком для отвлечения внимания.
     * Ослаблено стягивание в кольцо для средних/дальних роботов.
     */
    void ThrowWeaponCasingDistraction(const Vector3D& casingLandingPos);

    /**
     * @brief Нанесение покомпонентного урона (Метод боевого взаимодействия - возвращен)
     */
    void ApplyTargetedDamageToMonster(uint32_t monsterId, float damage, bool hitLegs);

    /**
     * @brief Триггер мгновенного оповещения роя дронов (вызывается, если один дрон увидел игрока)
     */
    void BroadcastSwarmTarget(const Vector3D& targetPos, uint32_t broadcastingDroneId);

    /**
     * @brief Спавн тактического агента на сцене
     */
    void SpawnTacticalAgent(uint32_t id, const std::string& loreName, AIArchetype archetype, const Vector3D& spawnPos);

    // Геттер для контекста Renderer3D
    [[nodiscard]] const std::vector<AIIntelligenceProfile>& GetActiveAgents() const noexcept { return m_simulatedAgents; }
};

} // namespace Centralia
