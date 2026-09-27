#include "video/Renderer3D.hpp"
#include "platform/Platform.hpp"

namespace Centralia {

static const float cubeVertices[] = {
    // Spatial Positions    // Surface Normals (Reflectance Calculations)
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

Renderer3D::Renderer3D(int width, int height) 
    : m_screenWidth(width), m_screenHeight(height) {}

Renderer3D::~Renderer3D() {
    Shutdown();
}

bool Renderer3D::Initialize(const char* windowTitle) {
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

    Platform::Log("Renderer3D: 3D Graphics Engine Context initialized successfully (Modern Hardware Pipeline active).");
    return true;
}

void Renderer3D::SetupCubeBuffers() {
    glGenVertexArrays(1, &m_cubeVAO);
    glGenBuffers(1, &m_cubeVBO);

    glBindVertexArray(m_cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    Platform::Log("Renderer3D: Allocating VBO/VAO vertex buffers for hardware-accelerated 3D meshes complete.");
}

void Renderer3D::ClearScreen() {
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer3D::Present() {
    SDL_GL_SwapWindow(m_window);
}

void Renderer3D::DrawTestCube(const Vector3D& pos, float rotationY) {
    glBindVertexArray(m_cubeVAO);
    
    // FIXED: Swapped out broken GL_TRIANGLE_FAN layout profile to parse valid raw quad primitives safely
#if defined(GL_QUADS)
    glDrawArrays(GL_QUADS, 0, 24); 
#else
    glDrawArrays(GL_TRIANGLES, 0, 24);
#endif
    
    glBindVertexArray(0);
}

void Renderer3D::Shutdown() {
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
}

bool Renderer3D::ShouldClose() {
    return Platform::WindowShouldClose();
}

} // namespace Centralia
