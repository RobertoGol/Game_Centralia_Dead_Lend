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

class ProceduralMotionManager {
private:
    ChassisType m_type;
    std::vector<ProceduralLeg> m_legs;
    float m_stepLength = 2.5f;
    float m_stepHeight = 0.8f;
    float m_stepSpeed = 4.0f;
    float m_chassisRoll = 0.0f;
    float m_chassisPitch = 0.0f;

    ProceduralMotionManager() noexcept;

public:
    ~ProceduralMotionManager() = default;
    ProceduralMotionManager(const ProceduralMotionManager&) = delete;
    ProceduralMotionManager& operator=(const ProceduralMotionManager&) = delete;

    static ProceduralMotionManager& GetInstance() noexcept {
        static ProceduralMotionManager instance;
        return instance;
    }

    void InitializeChassis(ChassisType type) noexcept;
    void UpdateTitanMovement(const Vector3D& bodyPosition, const Vector3D& moveDirection, float deltaTime) noexcept;

    [[nodiscard]] float GetChassisRoll() const noexcept { return m_chassisRoll; }
    [[nodiscard]] float GetChassisPitch() const noexcept { return m_chassisPitch; }
    [[nodiscard]] const std::vector<ProceduralLeg>& GetLegs() const noexcept { return m_legs; }
    [[nodiscard]] ChassisType GetChassisType() const noexcept { return m_type; }
};

    [[nodiscard]] inline float GetChassisRoll() const noexcept { return m_chassisRoll; }
    [[nodiscard]] inline float GetChassisPitch() const noexcept { return m_chassisPitch; }
    [[nodiscard]] inline const std::vector<ProceduralLeg>& GetLegs() const noexcept { return m_legs; }
    [[nodiscard]] inline ChassisType GetChassisType() const noexcept { return m_type; }
};

} // namespace Centralia
