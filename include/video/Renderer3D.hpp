#pragma once
#include "core/Math3D.hpp"
#include <SDL.h>
// Было: #include <SDL2/SDL.h>
#if defined(CENTRALIA_PLATFORM_WINDOWS)
    #include <windows.h>
    #include "glad/glad.h"

#elif defined(CENTRALIA_PLATFORM_ANDROID)
    #include "glad/glad.h"

#else
    #include <glad/glad.h> 
#endif

namespace Centralia {

class Renderer3D {
private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
    int m_screenWidth;
    int m_screenHeight;
    uint32_t m_cubeVAO = 0;
    uint32_t m_cubeVBO = 0;
    void SetupCubeBuffers(); // Метод генерации геометрии куба


public:
    Renderer3D(int width = 1920, int height = 1080);
    ~Renderer3D();

    // Создание окна и инициализация OpenGL контекста
    bool Initialize(const char* windowTitle);

    // Очистка экрана перед каждым кадром (черная Пустошь)
    void ClearScreen();

    // Обновление экрана (Swap buffers)
    void Present();

    // Рендеринг тестового 3D-куба на карте (вместо модели игрока/техники на этапе заготовки)
    void DrawTestCube(const Vector3D& position, float rotationY);

    void Shutdown();
    bool ShouldClose();
};

} // namespace Centralia
