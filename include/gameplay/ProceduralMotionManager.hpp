#pragma once
#include "core/Math3D.hpp"
#include "platform/Platform.hpp"
#include <vector>
#include <cstdint>
#include <cmath>

namespace Centralia {

enum class ChassisType : uint8_t {
    Titan_4_Legged,   
    Titan_Bipedal,    
    Crawler_Multileg, 
    Tracked_Tank      
};

struct ProceduralLeg {
    Vector3D hipOffset;       
    Vector3D currentFootPos;   
    Vector3D targetFootPos;    
    Vector3D startFootPos;     
    
    float stepProgress = 1.0f; 
    bool isMoving = false;     
};

class ProceduralMotionManager {
private:
    ChassisType m_type;
    std::vector<ProceduralLeg> m_legs;
    float m_stepLength = 2.5f;   
    float m_stepHeight = 0.8f;   
    float m_stepSpeed = 4.0f;    
    
    // Переменные наклона корпуса, которые жестко ищет твой main.cpp
    float m_chassisRoll = 0.0f;
    float m_chassisPitch = 0.0f;

    inline ProceduralMotionManager() noexcept {
        InitializeChassis(ChassisType::Titan_4_Legged);
    }

public:
    ~ProceduralMotionManager() = default;

    // Запрет копирования синглтона для защиты Ghost-RAM
    ProceduralMotionManager(const ProceduralMotionManager&) = delete;
    ProceduralMotionManager& operator=(const ProceduralMotionManager&) = delete;

    static inline ProceduralMotionManager& GetInstance() noexcept {
        static ProceduralMotionManager instance;
        return instance;
    }

    inline void InitializeChassis(ChassisType type) noexcept {
        m_type = type;
        m_legs.clear();

        if (m_type == ChassisType::Titan_4_Legged) {
            m_legs.resize(4);
            
            m_legs[0].hipOffset = Vector3D(-1.5f, 0.0f,  2.0f);  
            m_legs[1].hipOffset = Vector3D( 1.5f, 0.0f,  2.0f);  
            m_legs[2].hipOffset = Vector3D(-1.5f, 0.0f, -2.0f);  
            m_legs[3].hipOffset = Vector3D( 1.5f, 0.0f, -2.0f);  

            for (auto& leg : m_legs) {
                leg.currentFootPos = leg.hipOffset;
                leg.targetFootPos = leg.hipOffset;
                leg.startFootPos = leg.hipOffset; 
                leg.stepProgress = 1.0f;
                leg.isMoving = false;
            }
            
            Platform::Log("ProceduralMotion: Header-Only ИИК-шасси 4-х ногого Титана инициализировано.");
        }
    }

    inline void UpdateTitanMovement(const Vector3D& bodyPosition, const Vector3D& moveDirection, float deltaTime) noexcept {
        if (m_legs.empty()) return;

        float moveVelocity = moveDirection.Length();
        bool isMovingDirectional = (moveVelocity > 0.01f);

        for (size_t i = 0; i < m_legs.size(); ++i) {
            ProceduralLeg& leg = m_legs[i];
            Vector3D worldHipPos = bodyPosition + leg.hipOffset;

            if (!leg.isMoving && isMovingDirectional) {
                Vector3D idealTargetPos = worldHipPos + (moveDirection.Normalize() * m_stepLength);
                Vector3D deltaVector = idealTargetPos - leg.currentFootPos;
                
                bool otherLegsAnchored = true;
                for (size_t j = 0; j < m_legs.size(); ++j) {
                    if (i != j && m_legs[j].isMoving && (j % 2 == (i % 2))) {
                        otherLegsAnchored = false;
                    }
                }

                if (deltaVector.Length() > m_stepLength * 0.5f && otherLegsAnchored) {
                    leg.startFootPos = leg.currentFootPos; 
                    leg.targetFootPos = idealTargetPos;
                    leg.stepProgress = 0.0f;
                    leg.isMoving = true;
                }
            }

            if (leg.isMoving) {
                leg.stepProgress += m_stepSpeed * deltaTime;
                if (leg.stepProgress >= 1.0f) {
                    leg.stepProgress = 1.0f;
                    leg.currentFootPos = leg.targetFootPos;
                    leg.startFootPos = leg.targetFootPos; 
                    leg.isMoving = false;
                } else {
                    float heightArc = std::sin(leg.stepProgress * 3.14159265f) * m_stepHeight;
                    Vector3D currentPlanePos = leg.startFootPos + (leg.targetFootPos - leg.startFootPos) * leg.stepProgress;
                    leg.currentFootPos = Vector3D(currentPlanePos.x, currentPlanePos.y + heightArc, currentPlanePos.z);
                }
            }
        }
    }

    [[nodiscard]] inline float GetChassisRoll() const noexcept { return m_chassisRoll; }
    [[nodiscard]] inline float GetChassisPitch() const noexcept { return m_chassisPitch; }
    [[nodiscard]] inline const std::vector<ProceduralLeg>& GetLegs() const noexcept { return m_legs; }
    [[nodiscard]] inline ChassisType GetChassisType() const noexcept { return m_type; }
};

} // namespace Centralia
