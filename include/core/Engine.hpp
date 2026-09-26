#pragma once
#include <cstdint>
#include <vector>
#include "core/Math3D.hpp"
#include "core/MemoryManager.hpp"
#include "core/InputController.hpp"
#include "gameplay/Player.hpp"
#include "video/Renderer3D.hpp"
#include "video/Shader.hpp"


namespace Centralia {

// Специфические перечисления для логики классов и контроля админа
enum class ActiveControlMode : int32_t {
    Standard_Player = 0,
    Admin_Observer  = 1
};

class Engine {
private:
    bool            m_isRunning;
    MemoryManager   m_memoryManager;
    InputController m_inputController;
    Camera3D        m_camera;
    Shader          m_activeShader;
    
    // Объявляем строго как указатели, чтобы cl.exe не ругался на оператор delete
    Player*         m_localPlayer = nullptr;
    Renderer3D*     m_renderer = nullptr;

public:
    Engine();
    ~Engine();

    bool Start();
    void Update();
    void Render();
    void Stop();

    // Метод обработки мыши, который ищет линковщик на строке 56
    void HandleMouseMovement(float deltaX, float deltaY);

    [[nodiscard]] bool IsRunning() const noexcept { return m_isRunning; }
};

} // namespace Centralia
