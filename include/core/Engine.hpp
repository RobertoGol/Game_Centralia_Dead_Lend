#pragma once
#include <cstdint>
#include <vector>
 #include <chrono> // Для аппаратно-независимого подсчета дельты времени
#include <memory> // ИСПРАВЛЕНО: Добавлен заголовок для умных указателей
#include "core/Math3D.hpp" // Now safely yields ActiveControlMode and EngineControlMode
#include "core/MemoryManager.hpp"
#include "core/InputController.hpp"
#include "gameplay/ClassSystem.hpp" // Интегрируем для фикса строки 118

namespace Centralia {
class Player;
class Renderer3D;
class Shader; // ИСПРАВЛЕНО: Добавлено предварительное объявление класса

// FIXED: Using standard type validation to bypass the enum collision loop completely
#ifndef CENTRALIA_ENUMS_DECLARED
#define CENTRALIA_ENUMS_DECLARED
// Classes use the definitions compiled by Math3D.hpp safely
#endif

class Engine {
private:
    bool            m_isRunning;
    MemoryManager   m_memoryManager;
    InputController m_inputController;
    Camera3D        m_camera;
    ClassSystem     m_classSystem; // ИСПРАВЛЕНО: Объект вынесен на уровень класса (фикс строки 118)
    
    // Модернизированный programmable pipeline: шейдер загружается как Header-Only компонент
    // Если твой Shader.hpp переведен в Header-Only, объявление остается прежним
    uint32_t        m_activeShaderID = 0; 
    
    // ИСПРАВЛЕНО: Переведено на безопасные умные указатели
    std::unique_ptr<Player>     m_localPlayer;
    std::unique_ptr<Renderer3D> m_renderer;
    std::unique_ptr<Shader>     m_shader;

    // Тайминги для аппаратно-независимого счисления физики WASD
    std::chrono::high_resolution_clock::time_point m_lastFrameTime;

public:
    Engine();
    ~Engine();

    // Запрет копирования ядра движка
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    bool Start();
    void Update();
    void Render();
    void Stop();

    /**
     * @brief Обработчик сырого перемещения мыши для вращения 3D-камеры.
     * Вызывается из низкоуровневых абстракций событий Platform/SDL.
     */
    void HandleMouseMovement(float deltaX, float deltaY);

    [[nodiscard]] bool IsRunning() const noexcept { return m_isRunning; }
};

} // namespace Centralia
