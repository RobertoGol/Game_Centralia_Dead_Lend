#include "gameplay/PowerArmorSystem.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/MapSystem.hpp"
#include "gameplay/ItemDatabase.hpp"
#include "video/Renderer3D.hpp"
#include "platform/Platform.hpp"
#include "core/MemoryManager.hpp"
#include "core/InputController.hpp"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <fstream>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS, ENUMS & BALANCING TWEAKS
// ============================================================================

namespace PAConfig {
    constexpr uint32_t PA_SAVE_MAGIC = 0x50575241; // "PWRA"
    constexpr uint32_t PA_SAVE_VERSION = 2;

    // Расход энергии Ядерного Блока (Fusion Core) в условных единицах (Емкость 100.0)
    constexpr float DRAIN_RATE_IDLE = 0.005f;       // В секунду
    constexpr float DRAIN_RATE_WALK = 0.02f;        // В секунду
    constexpr float DRAIN_RATE_SPRINT = 0.15f;      // В секунду
    constexpr float DRAIN_RATE_JETPACK = 2.5f;      // В секунду
    constexpr float DRAIN_COST_JUMP = 0.5f;         // За один прыжок
    constexpr float DRAIN_COST_MELEE_HEAVY = 1.0f;  // За силовой удар
    
    // Физика
    constexpr float MASS_MULTIPLIER = 4.5f;         // Броня весит в 4.5 раза больше человека
    constexpr float FALL_DAMAGE_MULTIPLIER = 0.0f;  // Полный иммунитет к урону от падения
    constexpr float HEAVY_LANDING_VELOCITY = -15.0f;// Вертикальная скорость для активации ударной волны
    constexpr float CARRY_WEIGHT_BONUS = 200.0f;    // Бонус к переносимому весу
}

enum class PAPieceSlot {
    Helmet,
    Torso,
    LeftArm,
    RightArm,
    LeftLeg,
    RightLeg,
    Count
};

enum class PAAnimationState {
    None,
    EnteringOpenAnimation,
    EnteringStepIn,
    EnteringCloseAnimation,
    ExitingOpenAnimation,
    ExitingStepOut,
    ExitingCloseAnimation
};

// ============================================================================
// SECTION 2: DATA STRUCTURES (FRAMES, CORES & PIECES)
// ============================================================================

struct ArmorPieceData {
    uint32_t itemId;           // ID предмета из ItemDatabase
    float currentHealth;       // Текущая прочность
    float maxHealth;           // Максимальная прочность
    float armorRatingPhys;     // Защита от баллистики
    float armorRatingEnergy;   // Защита от лазеров
    float armorRatingRad;      // Защита от радиации
    bool isBroken;             // Деталь разрушена (статы = 0, не рендерится)
    uint32_t colorPaintId;     // ID покраски (Хот-Род, Братство Стали, Анклав)
};

struct FusionCoreData {
    uint32_t inventoryId;      // Уникальный ID предмета в инвентаре
    float remainingCharge;     // 0.0f - 100.0f
};

struct PowerArmorFrame {
    uint32_t frameInstanceId;
    Vector3D worldPosition;
    float yawRotation;
    
    bool isOccupied;
    uint32_t occupantEntityId; // ID Игрока или NPC
    
    FusionCoreData activeCore;
    ArmorPieceData pieces[static_cast<int>(PAPieceSlot::Count)];

    // Модификации эндоскелета
    bool hasJetpack;
    bool hasTargetingHUD;
    bool hasMedicPump;
    bool hasTeslaCoils;
};

// ============================================================================
// SECTION 3: SYSTEM IMPLEMENTATION & SINGLETON STATE
// ============================================================================

struct PowerArmorSystemImpl {
    uint32_t nextFrameId = 5000;
    std::unordered_map<uint32_t, PowerArmorFrame> spawnedFrames;

    // Состояние игрока
    bool isPlayerInArmor = false;
    uint32_t currentPlayerFrameId = 0;
    
    PAAnimationState currentAnimState = PAAnimationState::None;
    float animTimer = 0.0f;
    float headlampBattery = 100.0f;
    bool isHeadlampOn = false;

    // Телеметрия для HUD
    float hudVelocityY = 0.0f;
    float previousYPos = 0.0f;

    Player* playerRef = nullptr;
};

PowerArmorSystem* PowerArmorSystem::s_instance = nullptr;

PowerArmorSystem::PowerArmorSystem() : m_pImpl(new PowerArmorSystemImpl()) {
    if (s_instance) {
        Platform::Log("[POWER ARMOR FATAL]: Двойная инициализация системы!");
        std::terminate();
    }
    s_instance = this;
    Platform::Log("[POWER ARMOR SYSTEM]: Подсистема управления силовой броней загружена.");
}

PowerArmorSystem::~PowerArmorSystem() {
    m_pImpl->spawnedFrames.clear();
    delete m_pImpl;
    s_instance = nullptr;
    Platform::Log("[POWER ARMOR SYSTEM]: Память подсистемы освобождена.");
}

PowerArmorSystem& PowerArmorSystem::GetInstance() {
    if (!s_instance) std::terminate();
    return *s_instance;
}

void PowerArmorSystem::BindPlayer(Player* player) {
    m_pImpl->playerRef = player;
}

// ============================================================================
// SECTION 4: FRAME GENERATION & PIECE MANAGEMENT
// ============================================================================

uint32_t PowerArmorSystem::SpawnFrame(const Vector3D& position, float yaw) {
    uint32_t id = m_pImpl->nextFrameId++;
    
    PowerArmorFrame frame;
    frame.frameInstanceId = id;
    frame.worldPosition = position;
    frame.yawRotation = yaw;
    frame.isOccupied = false;
    frame.occupantEntityId = 0;
    
    frame.activeCore = {0, 0.0f}; // По умолчанию без ядра
    frame.hasJetpack = false;
    frame.hasTargetingHUD = false;
    frame.hasMedicPump = false;
    frame.hasTeslaCoils = false;

    for (int i = 0; i < static_cast<int>(PAPieceSlot::Count); ++i) {
        frame.pieces[i].itemId = 0;
        frame.pieces[i].isBroken = true;
    }

    m_pImpl->spawnedFrames[id] = frame;
    Platform::Log("[POWER ARMOR]: Новый пустой эндоскелет размещен. ID: " + std::to_string(id));
    
    return id;
}

bool PowerArmorSystem::AttachArmorPiece(uint32_t frameId, PAPieceSlot slot, uint32_t itemId) {
    auto it = m_pImpl->spawnedFrames.find(frameId);
    if (it == m_pImpl->spawnedFrames.end()) return false;

    PowerArmorFrame& frame = it->second;
    const ItemRecord* record = ItemDatabase::GetInstance().GetItemRecord(itemId);
    
    if (!record || record->type != ItemType::Armor) return false;

    ArmorPieceData newPiece;
    newPiece.itemId = itemId;
    newPiece.maxHealth = record->maxDurability;
    newPiece.currentHealth = record->maxDurability;
    newPiece.armorRatingPhys = record->armor.dr;
    newPiece.armorRatingEnergy = record->armor.er;
    newPiece.armorRatingRad = record->armor.rr;
    newPiece.isBroken = false;
    newPiece.colorPaintId = 0; // Standard rusted look

    frame.pieces[static_cast<int>(slot)] = newPiece;
    Platform::Log("[POWER ARMOR]: На эндоскелет ID " + std::to_string(frameId) + " установлена бронеплита " + record->name);
    
    return true;
}

bool PowerArmorSystem::InsertFusionCore(uint32_t frameId, uint32_t coreItemId, float chargePercent) {
    auto it = m_pImpl->spawnedFrames.find(frameId);
    if (it == m_pImpl->spawnedFrames.end()) return false;

    it->second.activeCore.inventoryId = coreItemId;
    it->second.activeCore.remainingCharge = std::clamp(chargePercent, 0.0f, 100.0f);
    
    Platform::Log("[POWER ARMOR]: Ядерный блок установлен. Уровень заряда: " + std::to_string(chargePercent) + "%");
    return true;
}

// ============================================================================
// SECTION 5: ENTER / EXIT STATE MACHINE (ANIMATION TIMINGS)
// ============================================================================

bool PowerArmorSystem::InitiateEnterArmor(uint32_t frameId) {
    if (m_pImpl->isPlayerInArmor || m_pImpl->currentAnimState != PAAnimationState::None) {
        return false;
    }

    auto it = m_pImpl->spawnedFrames.find(frameId);
    if (it == m_pImpl->spawnedFrames.end()) return false;

    PowerArmorFrame& frame = it->second;

    if (frame.isOccupied) {
        Platform::Log("[POWER ARMOR REJECT]: Броня уже занята другим персонажем.");
        return false;
    }

    if (frame.activeCore.remainingCharge <= 0.0f) {
        // Если ядра нет, запускаем анимацию вставки ядра сзади
        Platform::Log("[POWER ARMOR]: Нет энергии. Требуется вставить Ядерный Блок.");
        // Player checks inventory for core... (simplified here)
        return false;
    }

    // Запуск секвенции посадки (Блокировка управления)
    InputController::GetInstance().SetMouseCapture(false);
    // CameraSystem::MoveToThirdPersonView(frame.worldPosition - Vector3D(0, 0, 3));
    
    m_pImpl->currentPlayerFrameId = frameId;
    m_pImpl->currentAnimState = PAAnimationState::EnteringOpenAnimation;
    m_pImpl->animTimer = 0.0f;
    frame.isOccupied = true;
    frame.occupantEntityId = m_pImpl->playerRef->GetEntityId();

    // AudioSystem::PlaySound3D("sounds/pa_open_hatch.wav", frame.worldPosition);
    Platform::Log("[POWER ARMOR SEQUENCER]: Запуск кинематографической посадки в броню...");

    return true;
}

bool PowerArmorSystem::InitiateExitArmor() {
    if (!m_pImpl->isPlayerInArmor || m_pImpl->currentAnimState != PAAnimationState::None) {
        return false;
    }

    // Проверка, достаточно ли места сзади для выхода
    // RaycastHit hit;
    // if (PhysicsWorld::Raycast(m_pImpl->playerRef->GetPosition(), -m_pImpl->playerRef->GetForward(), 1.5f, hit)) {
    //     Platform::Log("[POWER ARMOR REJECT]: Недостаточно места для выхода (стена сзади).");
    //     return false;
    // }

    m_pImpl->currentAnimState = PAAnimationState::ExitingOpenAnimation;
    m_pImpl->animTimer = 0.0f;

    // AudioSystem::PlaySound3D("sounds/pa_release_valves.wav", m_pImpl->playerRef->GetPosition());
    Platform::Log("[POWER ARMOR SEQUENCER]: Запуск гидравлического открытия для выхода.");

    return true;
}

void PowerArmorSystem::ProcessAnimationState(float dt) {
    if (m_pImpl->currentAnimState == PAAnimationState::None) return;

    m_pImpl->animTimer += dt;

    switch (m_pImpl->currentAnimState) {
        
        // --- ENTERING SEQUENCE ---
        case PAAnimationState::EnteringOpenAnimation:
            if (m_pImpl->animTimer > 1.2f) {
                m_pImpl->currentAnimState = PAAnimationState::EnteringStepIn;
                m_pImpl->animTimer = 0.0f;
                // AudioSystem::PlaySound3D("sounds/pa_step_in.wav", m_pImpl->playerRef->GetPosition());
            }
            break;

        case PAAnimationState::EnteringStepIn:
            if (m_pImpl->animTimer > 0.8f) {
                m_pImpl->currentAnimState = PAAnimationState::EnteringCloseAnimation;
                m_pImpl->animTimer = 0.0f;
                // AudioSystem::PlaySound3D("sounds/pa_close_hatch.wav", m_pImpl->playerRef->GetPosition());
            }
            break;

        case PAAnimationState::EnteringCloseAnimation:
            if (m_pImpl->animTimer > 1.5f) {
                // ПОЛНАЯ ПЕРЕДАЧА УПРАВЛЕНИЯ БРОНЕ
                m_pImpl->isPlayerInArmor = true;
                m_pImpl->currentAnimState = PAAnimationState::None;
                
                InputController::GetInstance().SetMouseCapture(true);
                // CameraSystem::MoveToFirstPersonView(true); // True = PA HUD Overlay
                // AudioSystem::PlaySound2D("sounds/pa_hud_boot.wav"); // Звук загрузки CRT интерфейса
                
                ApplyPowerArmorPhysicsModifiers(true);
                Platform::Log("[POWER ARMOR SYSTEM]: Управление передано. Системы жизнеобеспечения онлайн.");
            }
            break;

        // --- EXITING SEQUENCE ---
        case PAAnimationState::ExitingOpenAnimation:
            if (m_pImpl->animTimer > 1.2f) {
                m_pImpl->currentAnimState = PAAnimationState::ExitingStepOut;
                m_pImpl->animTimer = 0.0f;
                // AudioSystem::PlaySound3D("sounds/pa_step_out.wav", m_pImpl->playerRef->GetPosition());
            }
            break;

        case PAAnimationState::ExitingStepOut:
            if (m_pImpl->animTimer > 0.8f) {
                m_pImpl->currentAnimState = PAAnimationState::ExitingCloseAnimation;
                m_pImpl->animTimer = 0.0f;
                
                // Перемещение игрока чуть назад
                // Vector3D ejectPos = m_pImpl->playerRef->GetPosition() - (m_pImpl->playerRef->GetForward() * 1.5f);
                // m_pImpl->playerRef->SetPosition(ejectPos);
            }
            break;

        case PAAnimationState::ExitingCloseAnimation:
            if (m_pImpl->animTimer > 1.0f) {
                m_pImpl->isPlayerInArmor = false;
                m_pImpl->currentAnimState = PAAnimationState::None;
                
                PowerArmorFrame& frame = m_pImpl->spawnedFrames[m_pImpl->currentPlayerFrameId];
                frame.isOccupied = false;
                frame.occupantEntityId = 0;
                // frame.worldPosition = m_pImpl->playerRef->GetPosition() + (m_pImpl->playerRef->GetForward() * 1.5f);
                
                m_pImpl->currentPlayerFrameId = 0;
                
                // CameraSystem::MoveToFirstPersonView(false);
                ApplyPowerArmorPhysicsModifiers(false);
                
                Platform::Log("[POWER ARMOR SYSTEM]: Выход из брони завершен. Эндоскелет оставлен в мире.");
            }
            break;
            
        default: break;
    }
}

// ============================================================================
// SECTION 6: PHYSICS OVERRIDES & HUD TELEMETRY
// ============================================================================

void PowerArmorSystem::ApplyPowerArmorPhysicsModifiers(bool isEntering) {
    if (!m_pImpl->playerRef) return;

    if (isEntering) {
        // Установка статов: Броня делает Силу равной 11 (или +Бонус)
        // m_pImpl->playerRef->SetOverrideStrength(11);
        
        // Увеличение массы для физического движка
        // PhysicsWorld::SetEntityMass(m_pImpl->playerRef->GetEntityId(), 85.0f * PAConfig::MASS_MULTIPLIER);
        
        // Иммунитет к урону от падения
        // m_pImpl->playerRef->SetFallDamageMultiplier(PAConfig::FALL_DAMAGE_MULTIPLIER);

        Platform::Log("[POWER ARMOR PHYSICS]: Активирована гидравлика. Иммунитет к падениям включен.");
    } else {
        // Возврат статов человека
        // m_pImpl->playerRef->RemoveOverrideStrength();
        // PhysicsWorld::SetEntityMass(m_pImpl->playerRef->GetEntityId(), 85.0f);
        // m_pImpl->playerRef->SetFallDamageMultiplier(1.0f);
        
        Platform::Log("[POWER ARMOR PHYSICS]: Гидравлика отключена. Возврат к человеческой физике.");
    }
}

void PowerArmorSystem::CheckHeavyLanding() {
    if (!m_pImpl->isPlayerInArmor || !m_pImpl->playerRef) return;

    // Расчет скорости приземления (разница высот за кадр)
    float currentY = m_pImpl->playerRef->GetPosition().y;
    m_pImpl->hudVelocityY = currentY - m_pImpl->previousYPos;
    m_pImpl->previousYPos = currentY;

    // Если персонаж стоял на земле после быстрого падения
    // bool isGrounded = m_pImpl->playerRef->IsGrounded();
    bool isGrounded = true; // Заглушка

    if (isGrounded && m_pImpl->hudVelocityY < (PAConfig::HEAVY_LANDING_VELOCITY * 0.016f)) { // 0.016f = deltaTime
        Platform::Log("[SUPERHERO LANDING]: Жесткое приземление! Генерация ударной волны...");
        
        // Screen Shake
        // CameraSystem::TriggerScreenShake(0.5f, 0.8f);

        // Звук удара металла о землю
        // AudioSystem::PlaySound3D("sounds/pa_heavy_land.wav", m_pImpl->playerRef->GetPosition());

        // Нанесение AoE урона врагам в радиусе
        float damageRadius = 5.0f;
        float aoeDamage = std::abs(m_pImpl->hudVelocityY) * 50.0f; // Урон скейлится от высоты падения
        
        // std::vector<Entity*> enemies = SpatialGrid::FindEnemiesInRange(m_pImpl->playerRef->GetPosition(), damageRadius);
        // for (auto* enemy : enemies) {
        //     DamageSystem::ApplyDamage(enemy->GetId(), aoeDamage);
        //     PhysicsWorld::ApplyImpulse(enemy->GetId(), (enemy->GetPosition() - m_pImpl->playerRef->GetPosition()).Normalized() * 500.0f); // Отбрасывание
        // }

        // Оглушение игрока на полсекунды (анимация подъема с колена)
        // m_pImpl->playerRef->Stun(0.5f);
    }
}

// ============================================================================
// SECTION 7: ENERGY DRAIN & FUSION CORE DEPLETION
// ============================================================================

void PowerArmorSystem::UpdateFusionCoreDrain(float dt) {
    if (!m_pImpl->isPlayerInArmor || m_pImpl->currentAnimState != PAAnimationState::None) return;

    PowerArmorFrame& frame = m_pImpl->spawnedFrames[m_pImpl->currentPlayerFrameId];
    FusionCoreData& core = frame.activeCore;

    if (core.remainingCharge <= 0.0f) {
        // Батарея пуста. Попытка автозамены
        if (!AutoReplaceFusionCore()) {
            // Энергия закончилась! Штраф к скорости 90%, отключение V.A.T.S.
            // m_pImpl->playerRef->SetSpeedMultiplier(0.1f);
            // CameraSystem::SetHUDWarning("FUSION CORE DEPLETED");
            return;
        }
    }

    // m_pImpl->playerRef->SetSpeedMultiplier(1.0f); // Возврат нормальной скорости

    float drainAmount = PAConfig::DRAIN_RATE_IDLE * dt;

    // Определение текущего состояния движения игрока (Бег, Шаг, Джетпак)
    // bool isSprinting = m_pImpl->playerRef->IsSprinting();
    // bool isJetpackActive = InputController::GetInstance().IsActionDown("Jump") && !m_pImpl->playerRef->IsGrounded();
    bool isSprinting = false;
    bool isJetpackActive = false;

    if (isJetpackActive && frame.hasJetpack) {
        drainAmount += PAConfig::DRAIN_RATE_JETPACK * dt;
        // ParticleSystem::Emit("JetpackThrust", m_pImpl->playerRef->GetPosition());
    } else if (isSprinting) {
        drainAmount += PAConfig::DRAIN_RATE_SPRINT * dt;
    } else {
        // Проверка на ходьбу
        // if (m_pImpl->playerRef->GetVelocity().LengthSquared() > 0.1f) {
        //     drainAmount += PAConfig::DRAIN_RATE_WALK * dt;
        // }
    }

    // Расход энергии на фонарик
    if (m_pImpl->isHeadlampOn) {
        drainAmount += 0.05f * dt;
    }

    core.remainingCharge -= drainAmount;

    // Обработка сингл-экшенов (Прыжок) - вызывается внешним ивентом, но здесь для примера
    // if (m_pImpl->playerRef->JustJumped()) core.remainingCharge -= PAConfig::DRAIN_COST_JUMP;
}

bool PowerArmorSystem::AutoReplaceFusionCore() {
    Platform::Log("[FUSION CORE]: Попытка автоматической замены ядерного блока...");
    
    // Поиск ядерного блока в инвентаре игрока
    // const auto& inventory = m_pImpl->playerRef->GetInventory();
    // for (size_t i = 0; i < inventory.size(); ++i) {
    //     if (inventory[i].itemId == 603) { // 603 = ID Ядерного блока
    //         PowerArmorFrame& frame = m_pImpl->spawnedFrames[m_pImpl->currentPlayerFrameId];
    //         frame.activeCore.remainingCharge = 100.0f;
    //         m_pImpl->playerRef->RemoveItem(i, 1);
    //         
    //         AudioSystem::PlaySound2D("sounds/pa_core_insert.wav");
    //         Platform::Log("[FUSION CORE]: Блок заменен успешно. Заряд 100%.");
    //         return true;
    //     }
    // }
    
    Platform::Log("[FUSION CORE WARNING]: Ядерные блоки не найдены в инвентаре!");
    return false;
}

// ============================================================================
// SECTION 8: LOCALIZED DAMAGE & ARMOR DEGRADATION
// ============================================================================

void PowerArmorSystem::ApplyLocalizedDamage(PAPieceSlot targetSlot, float damageAmount) {
    if (!m_pImpl->isPlayerInArmor) return;

    PowerArmorFrame& frame = m_pImpl->spawnedFrames[m_pImpl->currentPlayerFrameId];
    ArmorPieceData& piece = frame.pieces[static_cast<int>(targetSlot)];

    if (piece.isBroken || piece.itemId == 0) {
        // Урон проходит напрямую по игроку (броня пробита)
        // m_pImpl->playerRef->ApplyDamage(damageAmount);
        Platform::Log("[POWER ARMOR PENETRATION]: Броня разрушена на этом участке. Игрок получает прямой урон!");
        return;
    }

    // Поглощение урона физикой брони
    // float mitigatedDamage = ItemDatabase::GetInstance().CalculateArmorReduction(piece.itemId, damageAmount, 0.0f);
    float mitigatedDamage = damageAmount * 0.3f; // Заглушка

    // Отнимаем прочность самой детали
    piece.currentHealth -= damageAmount * 2.0f; // Броня ломается быстрее, чем игрок

    if (piece.currentHealth <= 0.0f) {
        piece.currentHealth = 0.0f;
        piece.isBroken = true;
        
        Platform::Log("[POWER ARMOR BREAK]: Часть брони РАЗРУШЕНА! Участок: " + std::to_string(static_cast<int>(targetSlot)));
        
        // Звук ломающегося металла и визуальный эффект искр
        // AudioSystem::PlaySound3D("sounds/pa_armor_break.wav", m_pImpl->playerRef->GetPosition());
        // ParticleSystem::Emit("ArmorSparks", m_pImpl->playerRef->GetPosition());
        
        // Отстрел куска брони как физического объекта (Havok/PhysX)
        // PhysicsWorld::SpawnDebris(piece.itemId, m_pImpl->playerRef->GetPosition());
    } else {
        // Осколочный урон по игроку (сквозь броню)
        // m_pImpl->playerRef->ApplyDamage(mitigatedDamage);
    }
}

// ============================================================================
// SECTION 9: GLOBAL UPDATE TICK & AUDIO MANAGEMENT
// ============================================================================

void PowerArmorSystem::UpdateTick(float deltaTime) {
    // 1. Анимации посадки/высадки
    ProcessAnimationState(deltaTime);

    if (m_pImpl->isPlayerInArmor && m_pImpl->currentAnimState == PAAnimationState::None) {
        // 2. Расход энергии
        UpdateFusionCoreDrain(deltaTime);

        // 3. Расчет приземлений (Superhero Landing)
        CheckHeavyLanding();

        // 4. Логика Фонарика (Headlamp)
        // Если фонарь включен - рисуем Volumetric Cone перед игроком
        if (m_pImpl->isHeadlampOn) {
            // Renderer3D::DrawSpotLight(m_pImpl->playerRef->GetPosition() + Vector3D(0, 1.8f, 0), m_pImpl->playerRef->GetForward(), Vector3D(1.0f, 1.0f, 0.9f));
        }

        // 5. Воспроизведение гидравлических шагов
        // В реальном движке это привязано к Animation Events в `ProceduralMotion.cpp`
        // if (m_pImpl->playerRef->IsFootstepFrame()) {
        //     AudioSystem::PlaySound3D("sounds/pa_footstep_heavy.wav", m_pImpl->playerRef->GetPosition());
        //     CameraSystem::TriggerScreenShake(0.05f, 0.1f); // Легкая тряска при ходьбе
        // }
    }
}

void PowerArmorSystem::ToggleHeadlamp() {
    if (!m_pImpl->isPlayerInArmor) return;
    
    m_pImpl->isHeadlampOn = !m_pImpl->isHeadlampOn;
    // AudioSystem::PlaySound2D("sounds/pa_flashlight_click.wav");
    Platform::Log(std::string("[POWER ARMOR]: Прожектор шлема ") + (m_pImpl->isHeadlampOn ? "ВКЛЮЧЕН" : "ВЫКЛЮЧЕН"));
}

bool PowerArmorSystem::IsPlayerInArmor() const noexcept {
    return m_pImpl->isPlayerInArmor;
}

float PowerArmorSystem::GetFusionCoreCharge() const noexcept {
    if (!m_pImpl->isPlayerInArmor) return 0.0f;
    return m_pImpl->spawnedFrames.at(m_pImpl->currentPlayerFrameId).activeCore.remainingCharge;
}

// ============================================================================
// SECTION 10: BINARY SERIALIZATION (WORLD SAVING)
// ============================================================================

uint32_t PowerArmorSystem::CalculateChecksum(const std::vector<uint8_t>& buffer) const noexcept {
    uint32_t crc = 0xFFFFFFFF;
    for (uint8_t byte : buffer) {
        crc ^= byte;
        for (int i = 0; i < 8; ++i) {
            crc = (crc >> 1) ^ (0xEDB88320 & (-(crc & 1)));
        }
    }
    return ~crc;
}

std::vector<uint8_t> PowerArmorSystem::SerializeToBinary() const {
    std::vector<uint8_t> buffer;
    buffer.reserve(1024 * 10);

    const uint8_t* magicPtr = reinterpret_cast<const uint8_t*>(&PAConfig::PA_SAVE_MAGIC);
    buffer.insert(buffer.end(), magicPtr, magicPtr + sizeof(uint32_t));

    const uint8_t* verPtr = reinterpret_cast<const uint8_t*>(&PAConfig::PA_SAVE_VERSION);
    buffer.insert(buffer.end(), verPtr, verPtr + sizeof(uint32_t));

    // ID каунтер
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_pImpl->nextFrameId), reinterpret_cast<const uint8_t*>(&m_pImpl->nextFrameId) + sizeof(uint32_t));

    // Сохранение разбросанных по миру эндоскелетов
    uint32_t frameCount = static_cast<uint32_t>(m_pImpl->spawnedFrames.size());
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&frameCount), reinterpret_cast<const uint8_t*>(&frameCount) + sizeof(uint32_t));

    for (const auto& [id, frame] : m_pImpl->spawnedFrames) {
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&frame.frameInstanceId), reinterpret_cast<const uint8_t*>(&frame.frameInstanceId) + sizeof(uint32_t));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&frame.worldPosition), reinterpret_cast<const uint8_t*>(&frame.worldPosition) + sizeof(Vector3D));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&frame.yawRotation), reinterpret_cast<const uint8_t*>(&frame.yawRotation) + sizeof(float));
        
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&frame.isOccupied), reinterpret_cast<const uint8_t*>(&frame.isOccupied) + sizeof(bool));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&frame.occupantEntityId), reinterpret_cast<const uint8_t*>(&frame.occupantEntityId) + sizeof(uint32_t));

        // Core
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&frame.activeCore), reinterpret_cast<const uint8_t*>(&frame.activeCore) + sizeof(FusionCoreData));
        
        // Flags
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&frame.hasJetpack), reinterpret_cast<const uint8_t*>(&frame.hasJetpack) + sizeof(bool));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&frame.hasTargetingHUD), reinterpret_cast<const uint8_t*>(&frame.hasTargetingHUD) + sizeof(bool));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&frame.hasMedicPump), reinterpret_cast<const uint8_t*>(&frame.hasMedicPump) + sizeof(bool));
        buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&frame.hasTeslaCoils), reinterpret_cast<const uint8_t*>(&frame.hasTeslaCoils) + sizeof(bool));

        // Armor Pieces
        for (int i = 0; i < static_cast<int>(PAPieceSlot::Count); ++i) {
            const ArmorPieceData& p = frame.pieces[i];
            buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&p), reinterpret_cast<const uint8_t*>(&p) + sizeof(ArmorPieceData));
        }
    }

    // State игрока
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_pImpl->isPlayerInArmor), reinterpret_cast<const uint8_t*>(&m_pImpl->isPlayerInArmor) + sizeof(bool));
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&m_pImpl->currentPlayerFrameId), reinterpret_cast<const uint8_t*>(&m_pImpl->currentPlayerFrameId) + sizeof(uint32_t));

    // CRC32
    uint32_t checksum = CalculateChecksum(buffer);
    buffer.insert(buffer.end(), reinterpret_cast<const uint8_t*>(&checksum), reinterpret_cast<const uint8_t*>(&checksum) + sizeof(uint32_t));

    Platform::Log("[POWER ARMOR SERIALIZE]: Все брошенные и активные эндоскелеты сохранены в дамп.");
    return buffer;
}

bool PowerArmorSystem::DeserializeFromBinary(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < sizeof(uint32_t) * 4) return false;

    size_t payloadSize = buffer.size() - sizeof(uint32_t);
    std::vector<uint8_t> payloadData(buffer.begin(), buffer.begin() + payloadSize);
    uint32_t expectedChecksum = CalculateChecksum(payloadData);

    uint32_t storedChecksum = 0;
    std::memcpy(&storedChecksum, buffer.data() + payloadSize, sizeof(uint32_t));

    if (expectedChecksum != storedChecksum) {
        Platform::Log("[POWER ARMOR DESERIALIZE ERROR]: Искажение файла (CRC32 Mismatch).");
        return false;
    }

    size_t cursor = 0;
    uint32_t magic;
    std::memcpy(&magic, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
    if (magic != PAConfig::PA_SAVE_MAGIC) return false;

    uint32_t version;
    std::memcpy(&version, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

    std::memcpy(&m_pImpl->nextFrameId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

    uint32_t frameCount;
    std::memcpy(&frameCount, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

    m_pImpl->spawnedFrames.clear();
    for (uint32_t i = 0; i < frameCount; ++i) {
        PowerArmorFrame frame;
        
        std::memcpy(&frame.frameInstanceId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);
        std::memcpy(&frame.worldPosition, buffer.data() + cursor, sizeof(Vector3D)); cursor += sizeof(Vector3D);
        std::memcpy(&frame.yawRotation, buffer.data() + cursor, sizeof(float)); cursor += sizeof(float);
        
        std::memcpy(&frame.isOccupied, buffer.data() + cursor, sizeof(bool)); cursor += sizeof(bool);
        std::memcpy(&frame.occupantEntityId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

        std::memcpy(&frame.activeCore, buffer.data() + cursor, sizeof(FusionCoreData)); cursor += sizeof(FusionCoreData);
        
        std::memcpy(&frame.hasJetpack, buffer.data() + cursor, sizeof(bool)); cursor += sizeof(bool);
        std::memcpy(&frame.hasTargetingHUD, buffer.data() + cursor, sizeof(bool)); cursor += sizeof(bool);
        std::memcpy(&frame.hasMedicPump, buffer.data() + cursor, sizeof(bool)); cursor += sizeof(bool);
        std::memcpy(&frame.hasTeslaCoils, buffer.data() + cursor, sizeof(bool)); cursor += sizeof(bool);

        for (int j = 0; j < static_cast<int>(PAPieceSlot::Count); ++j) {
            std::memcpy(&frame.pieces[j], buffer.data() + cursor, sizeof(ArmorPieceData));
            cursor += sizeof(ArmorPieceData);
        }

        m_pImpl->spawnedFrames[frame.frameInstanceId] = frame;
    }

    std::memcpy(&m_pImpl->isPlayerInArmor, buffer.data() + cursor, sizeof(bool)); cursor += sizeof(bool);
    std::memcpy(&m_pImpl->currentPlayerFrameId, buffer.data() + cursor, sizeof(uint32_t)); cursor += sizeof(uint32_t);

    Platform::Log("[POWER ARMOR DESERIALIZE]: Успешное восстановление состояния силовых бронекостюмов.");
    return true;
}

} // namespace Centralia