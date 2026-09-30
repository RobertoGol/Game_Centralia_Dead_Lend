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
    
    float m_chassisRoll = 0.0f;
    float m_chassisPitch = 0.0f;

    ProceduralMotionManager() noexcept {
        InitializeChassis(ChassisType::Titan_4_Legged);
    }

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
    [[nodiscard]] float GetChassisPitch() const { return m_chassisPitch; }
    [[nodiscard]] const std::vector<ProceduralLeg>& GetLegs() const noexcept { return m_legs; }
    [[nodiscard]] ChassisType GetChassisType() const noexcept { return m_type; }
};

} // namespace Centralia