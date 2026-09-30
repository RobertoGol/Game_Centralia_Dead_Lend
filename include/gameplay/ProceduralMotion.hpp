#pragma once
#include "core/Math3D.hpp"
#include "platform/Platform.hpp"
#include <vector>
#include <cstdint>

namespace Centralia {

struct IKChainNode {
    Vector3D position;
    float length;
    float currentAngle;
};

class ProceduralMotion {
private:
    std::vector<IKChainNode> m_nodes;
    float m_totalReach;

public:
    ProceduralMotion();
    ~ProceduralMotion() = default;

    ProceduralMotion(const ProceduralMotion&) = delete;
    ProceduralMotion& operator=(const ProceduralMotion&) = delete;

    void AddNode(const Vector3D& pos, float length);
    bool SolveIK(const Vector3D& targetPosition);

    [[nodiscard]] const std::vector<IKChainNode>& GetNodes() const noexcept { return m_nodes; }
    [[nodiscard]] float GetTotalReach() const noexcept { return m_totalReach; }
};

} // namespace Centralia