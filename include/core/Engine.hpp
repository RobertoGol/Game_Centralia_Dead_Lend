#pragma once
#include <memory>
#include <string>
#include <cstdint>
#include "platform/Platform.hpp"
#include "video/Renderer3D.hpp"
#include "core/InputController.hpp"

namespace Centralia {

class Engine {
private:
    bool m_isRunning;
    std::unique_ptr<Renderer3D> m_renderer;
    std::unique_ptr<InputController> m_inputController;
    
    float m_deltaTime;
    uint64_t m_lastFrameTime;

public:
    Engine();
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool Initialize(const std::string& windowTitle, int width, int height);
    void Run();
    void Update(float deltaTime);
    void Render();
    void Shutdown();

    [[nodiscard]] bool IsRunning() const noexcept { return m_isRunning; }
    void RequestQuit() noexcept { m_isRunning = false; }
};

} // namespace Centralia