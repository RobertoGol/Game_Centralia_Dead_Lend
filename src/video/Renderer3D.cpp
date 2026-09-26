#include "video/Renderer3D.hpp"
#include "platform/Platform.hpp"

namespace Centralia {

// Массив вершин честного 3D-куба в пространстве (X, Y, Z) + направления нормалей (NX, NY, NZ) для расчета отражений
static const float cubeVertices[] = {
    // Позиции вершин      // Нормали полигонов (куда смотрит грань)
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
    // 1. Инициализация видео-подсистемы SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        Platform::Log("SDL Could not initialize! Error: " + std::string(SDL_GetError()));
        return false;
    }

    // 2. Настройка профиля OpenGL Core 3.3 для баланса GPU/CPU
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // 3. Создание кроссплатформенного окна
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

    // 4. Привязка OpenGL к окну
    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        Platform::Log("OpenGL context could not be created! Error: " + std::string(SDL_GetError()));
        return false;
    }

    // Инициализируем GLAD, чтобы Windows 10 увидела функции буферов видеокарты
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        Platform::Log("Critical Error: GLAD failed to link GPU drivers!");
        return false;
    }

    glEnable(GL_DEPTH_TEST); // Включаем буфер глубины (Z-буфер)
    glDepthFunc(GL_LESS);
    glViewport(0, 0, m_screenWidth, m_screenHeight);

    // Вызываем аппаратное выделение буферов в видеопамяти
    SetupCubeBuffers();

    Platform::Log("Renderer3D: 3D Graphics Engine Context initialized successfully (Modern Hardware Pipeline active).");
    return true;
}

void Renderer3D::SetupCubeBuffers() {
    // Генерируем контейнеры в памяти видеокарты
    glGenVertexArrays(1, &m_cubeVAO);
    glGenBuffers(1, &m_cubeVBO);

    // Биндим VAO (включаем запись конфигурации для GPU)
    glBindVertexArray(m_cubeVAO);

    // Загружаем массив вершин куба в буфер VBO
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

    // Позиции вершин (layout = 0 в шейдере) — шаг 6 байт, смещение 0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Нормали граней для отражений (layout = 1 в шейдере) — шаг 6 байт, смещение 3 плавающих точки
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Отвязываем буферы, они успешно сохранены в VRAM видеокарты
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
    // В современном OpenGL Core процессору больше не нужно слать геометрию каждый кадр.
    // Мы просто говорим видеокарте: «Включи буфер куба №m_cubeVAO и нарисуй его».
    glBindVertexArray(m_cubeVAO);
    
    // Отрисовываем 24 вершины (6 граней по 4 вершины) одной быстрой аппаратной командой
    glDrawArrays(GL_TRIANGLE_FAN, 0, 24); 
    
    glBindVertexArray(0);
}

void Renderer3D::Shutdown() {
    // Чистим память видеокарты при выходе из игры
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

} // namespace Centralia
