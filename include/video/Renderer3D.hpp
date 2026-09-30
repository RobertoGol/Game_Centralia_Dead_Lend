#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "core/Math3D.hpp"
#include "platform/Platform.hpp"

namespace Centralia {

enum class RenderPipelineMode : uint8_t {
    Forward,
    Deferred,
    RayTracingHybrid
};

struct RenderStats {
    uint32_t drawCalls;
    uint32_t triangleCount;
    float frameTimeMs;
    float gpuMemoryUsedMB;
};

class Renderer3D {
private:
    RenderPipelineMode m_pipelineMode;
    uint32_t m_viewportWidth;
    uint32_t m_viewportHeight;
    bool m_isInitialized;
    RenderStats m_lastStats;

public:
    Renderer3D();
    ~Renderer3D();

    Renderer3D(const Renderer3D&) = delete;
    Renderer3D& operator=(const Renderer3D&) = delete;

    bool Initialize(uint32_t width, uint32_t height, RenderPipelineMode mode);
    void BeginFrame();
    void EndFrame();
    void Resize(uint32_t width, uint32_t height);

    void SetPipelineMode(RenderPipelineMode mode) noexcept { m_pipelineMode = mode; }
    [[nodiscard]] RenderPipelineMode GetPipelineMode() const noexcept { return m_pipelineMode; }
    [[nodiscard]] const RenderStats& GetRenderStats() const noexcept { return m_lastStats; }
    [[nodiscard]] bool IsInitialized() const noexcept { return m_isInitialized; }
};

} // namespace Centralia