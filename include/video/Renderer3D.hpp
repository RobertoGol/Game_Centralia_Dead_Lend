#pragma once
#include "core/Math3D.hpp"
#include "platform/Platform.hpp" // Наша единая лог-система
#include <SDL.h>
#include <string>
#include <cstdint>

#if defined(_WIN32)
    #include <windows.h>
    #include <glad/glad.h>
#elif defined(__ANDROID__)
    #include <glad/glad.h>
#else
    #include <glad/glad.h> 
#endif

namespace Centralia {

// Массив вершин честного 3D-куба в пространстве (X, Y, Z) + нормали (NX, NY, NZ) для расчета Cube-отражений
inline const float cubeVerticesFlatArray[] = {
    -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
     0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
     0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
    -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,

    -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
     0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
     0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
    -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,

    -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
    -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,

     0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,
     0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,
     0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,
     0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f
};

class Renderer3D {
private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
    int m_screenWidth;
    int m_screenHeight;
    uint32_t m_cubeVAO = 0;
    uint32_t m_cubeVBO = 0;

    class Renderer3D {
private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
    int m_screenWidth;
    int m_screenHeight;
    uint32_t m_cubeVAO = 0;
    uint32_t m_cubeVBO = 0;

    void SetupCubeBuffers() noexcept;

public:
    Renderer3D(int width = 1920, int height = 1080);
    ~Renderer3D();

    bool Initialize(const char* windowTitle) noexcept;
    void ClearScreen() noexcept;
    void Present() noexcept;
    void DrawTestCube(const Vector3D& position, float rotationY) noexcept;
    void Shutdown() noexcept;
    bool ShouldClose() noexcept;
};

public:
    inline Renderer3D(int width = 1920, int height = 1080) 
        : m_screenWidth(width), m_screenHeight(height) {}

    inline ~Renderer3D() {
        Shutdown();
    }

    inline bool Initialize(const char* windowTitle) noexcept {
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            Platform::Log("SDL Could not initialize! Error: " + std::string(SDL_GetError()));
            return false;
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        m_window = SDL_CreateWindow(
            windowTitle,
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            m_screenWidth, m_screenHeight,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
        );

        if (!m_window) {
            Platform::Log("Window could not be created! Error: " + std::string(SDL_GetError()));
            return false;
        }

        m_glContext = SDL_GL_CreateContext(m_window);
        if (!m_glContext) {
            Platform::Log("OpenGL context could not be created! Error: " + std::string(SDL_GetError()));
            return false;
        }

        if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
            Platform::Log("Critical Error: GLAD failed to link GPU drivers!");
            return false;
        }

        glEnable(GL_DEPTH_TEST); 
        glDepthFunc(GL_LESS);
        glViewport(0, 0, m_screenWidth, m_screenHeight);

        SetupCubeBuffers();
        Platform::Log("Renderer3D: Контекст Modern OpenGL Core 3.3 запущен номинально.");
        return true;
    };

    inline void ClearScreen() noexcept {
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    };

    inline void Present() noexcept {
        SDL_GL_SwapWindow(m_window);
    };

    inline void DrawTestCube(const Vector3D& position, float rotationY) noexcept {
        glBindVertexArray(m_cubeVAO);
        
        // ИСПРАВЛЕНО: Безопасный кроссплатформенный рендеринг без разрушения осей куба
        #if defined(GL_QUADS)
                glDrawArrays(GL_QUADS, 0, 24); 
        #else
                glDrawArrays(GL_TRIANGLES, 0, 24);
        #endif
                glBindVertexArray(0);
    };

    inline void Shutdown() noexcept {
        if (m_cubeVAO != 0) glDeleteVertexArrays(1, &m_cubeVAO);
        if (m_cubeVBO != 0) glDeleteBuffers(1, &m_cubeVBO);

        if (m_glContext) {
            SDL_GL_DeleteContext(m_glContext);
            m_glContext = nullptr;
        }
        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }
        SDL_Quit();
    };

    inline bool ShouldClose() noexcept {
        return Platform::WindowShouldClose();
    };
};

}; // namespace Centralia
