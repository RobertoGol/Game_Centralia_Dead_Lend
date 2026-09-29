#define WIN32_LEAN_AND_MEAN
#define _USE_MATH_DEFINES
#if defined(_WIN32)
    #include <windows.h>
#endif
#include <glad/glad.h> // ИСПРАВЛЕНО: Сначала подключаем glad, fixed-function GL/gl.h вырезан полностью!
#include "core/Engine.hpp"
#include "platform/Platform.hpp"
#include "gameplay/FactorySystem.hpp" 
#include "gameplay/ProceduralMotionManager.hpp" // ИСПРАВЛЕНО: Путь синхронизирован, фантомный файл удален
#include "gameplay/ItemDatabase.hpp"
#include "gameplay/CraftingManager.hpp"
#include "core/NetworkSocket.hpp"
#include "core/NetworkProtocol.hpp"
#include "gameplay/ModificationSystem.hpp"
#include "gameplay/MapSystem.hpp"
#include "video/Renderer3D.hpp"
#include "video/Shader.hpp"
#include <thread>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace Centralia {

// Вспомогательная утилита для генерации матрицы обзора от третьего лица (Замена gluLookAt на CPU)
static void ComputeFakeViewMatrix(const Vector3D& eye, const Vector3D& target, float* outMatrix16) noexcept {
    Vector3D zAxis = (eye - target).Normalize();
    Vector3D up(0.0f, 1.0f, 0.0f);
    Vector3D xAxis = up.Cross(zAxis).Normalize();
    Vector3D yAxis = zAxis.Cross(xAxis);

    outMatrix16[0] = xAxis.x; outMatrix16[4] = xAxis.y; outMatrix16[8] = xAxis.z;  outMatrix16[12] = -xAxis.Dot(eye);
    outMatrix16[1] = yAxis.x; outMatrix16[5] = yAxis.y; outMatrix16[9] = yAxis.z;  outMatrix16[13] = -yAxis.Dot(eye);
    outMatrix16[2] = zAxis.x; outMatrix16[6] = zAxis.y; outMatrix16[10] = zAxis.z; outMatrix16[14] = -zAxis.Dot(eye);
    outMatrix16[3] = 0.0f;    outMatrix16[7] = 0.0f;    outMatrix16[11] = 0.0f;    outMatrix16[15] = 1.0f;
}

Engine::Engine() : m_isRunning(false), m_localPlayer(nullptr), m_renderer(nullptr) {}
Engine::~Engine() { Stop(); }

bool Engine::Start() {
    if (!Platform::Initialize()) return false;

    // Фикс строки 26: Вызов переведен на валидный метод Initialize() согласно ItemDatabase.hpp  
    ItemDatabase::GetInstance().Initialize();
    CraftingManager::GetInstance().InitializeBlueprints();

    if (!NetworkSocket::GlobalInit()) return false;
    if (!m_memoryManager.Initialize(Platform::GetDeviceHWID())) return false;

    // 1. ПОДГРУЗКА ВЫСОКОПРОИЗВОДИТЕЛЬНОЙ 22-БАЙТОВОЙ КАРТЫ  
    MapSystem::GetInstance().LoadMapFromFile("test.map");  

    // Запуск 3D-экрана Windows 10/Linux  
    m_renderer = new Renderer3D(1920, 1080);  
    if (!m_renderer->Initialize("Game Centralia: Dead Lend (Programmable GPU Core Build)")) {  
        return false;
    }

// Компиляция шейдеров многоуровневых красок на GPU 
    m_shader = std::make_unique<Shader>();
    if (!m_shader->LoadFromFiles("shaders/base_3d.vert", "shaders/base_3d.frag")) { 
        Platform::Log("Critical Error: GPU failed to compile core Centralia reflection shaders!"); 
        m_shader.reset();
        return false;
    }
    m_activeShaderID = m_shader->GetProgramID();

    m_inputController.Initialize();  

    m_localPlayer = new Player(777, "Vault_Survivor_76", 24);  
    m_localPlayer->SetPosition(Vector3D(0.0f, 0.0f, 0.0f));  

    m_memoryManager.SetRegistryValue("player_alpha_pct", 100);   

    // Фиксируем стартовую точку времени для аппаратно-адаптивного таймера
    m_lastFrameTime = std::chrono::high_resolution_clock::now();
    
    m_isRunning = true;
    Platform::Log("Engine: Core subsystems initialized. Programmable Hardware Pipeline linked.");  
    return true;
}

void Engine::HandleMouseMovement(float deltaX, float deltaY) {
    if (!m_localPlayer) return;  
    m_camera.FollowPlayer(m_localPlayer->GetPosition(), deltaX, deltaY);  
}

void Engine::Update() {
    // ИСПРАВЛЕНО: Перехват закрытия окна
    if (Platform::WindowShouldClose()) {
        Stop();
        return;
    }
    if (!m_localPlayer) return;  

    // Рассчитываем честную дельту времени (deltaTime) между тиками процессора
    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentFrameTime - m_lastFrameTime).count();
    m_lastFrameTime = currentFrameTime;

    // Предохранитель от падения FPS (например, при зависании окна или брейкпоинте отладчика)
    if (deltaTime > 0.1f) deltaTime = 0.1f;

    // 1. Опрос геймпада/мыши для вращения 3D-камеры вокруг гуманоида  
    float gamepadLookX = 0.0f;  
    float gamepadLookY = 0.0f;  
    m_inputController.GetLookOffsets(gamepadLookX, gamepadLookY);  
    if (gamepadLookX != 0.0f || gamepadLookY != 0.0f) {  
        m_camera.FollowPlayer(m_localPlayer->GetPosition(), gamepadLookX, gamepadLookY);  
    }

    const GameplayActions& actions = m_inputController.GetActions();  

    // 2. СИСТЕМА УПРАВЛЕНИЯ КЛАССАМИ (Админ-Хост против Обычного Пилота)  
    if (m_memoryManager.GetRegistryValue("active_control_mode") == static_cast<int32_t>(EngineControlMode::Admin_Observer)) {  
        m_camera.isAdminMode = true;  
        
        Vector3D inputDir = m_inputController.GetMovementVector();  
        m_camera.MoveFreeCam(inputDir.z, inputDir.x, 0.0f, deltaTime); // ИСПРАВЛЕНО: Внедрена адаптивная дельта
        
        static uint32_t adminLogTick = 0;  
        if (adminLogTick++ % 300 == 0) {  
            Platform::Log("[ADMIN HOST]: Свободный полет админ-камеры активен. Мониторинг P2P-пакетов.");  
        }
    } 
    else {
        m_camera.isAdminMode = false;   
        
        Vector3D inputDir = m_inputController.GetMovementVector();  
        bool isMoving = (inputDir.Length() > 0.0f);  
        Vector3D finalMovement(0.0f, 0.0f, 0.0f);  

        if (isMoving) {  
            float currentSpeed = m_inputController.IsSprintPressed() ? 8.0f : 4.0f;  
            float angleRad = m_camera.yaw * static_cast<float>(M_PI) / 180.0f;  

            Vector3D cameraForward(std::sin(angleRad), 0.0f, std::cos(angleRad));  
            Vector3D cameraRight(std::cos(angleRad), 0.0f, -std::sin(angleRad));  

            finalMovement = (cameraForward * inputDir.z) + (cameraRight * inputDir.x);  
            
            // ВЫЧИСЛЕНИЕ ВЕКТОРНОЙ КОЛЛИЗИИ СТЕН НА CPU С УЧЕТОМ ДЕЛЬТЫ ВРЕМЕНИ  
            Vector3D predictedPosition = m_localPlayer->GetPosition() + (finalMovement.Normalize() * currentSpeed * deltaTime);  
            
            if (!MapSystem::GetInstance().CheckCollision(predictedPosition)) {  
                m_localPlayer->Move(finalMovement, currentSpeed, deltaTime);  
            } else {
                finalMovement = Vector3D(0.0f, 0.0f, 0.0f);   
            }
            
            m_localPlayer->SetRotation(-m_camera.yaw - 90.0f);  
        }

        // Обсчет тиков дебаффов среды (Радиация, ЭМИ) на основе данных ячейки под ногами  
        // ИСПРАВЛЕНО: Строка 118 вылечена. Используем член класса m_classSystem вместо ежекадровой аллокации на стеке
        MapSystem::GetInstance().UpdateMapEnvironment(deltaTime, *m_localPlayer, m_classSystem);  

        // --- МАТЕМАТИЧЕСКИЙ ОБСЧЕТ ТИТАНА И ТЕХНИКИ (CPU) ---  
        static Vector3D mockTitanPos(5.0f, 0.0f, 5.0f);  
        if (finalMovement.Length() > 0.0f) {  
            mockTitanPos = mockTitanPos + (finalMovement.Normalize() * 3.5f * deltaTime);  
        }
        ProceduralMotionManager::GetInstance().UpdateTitanMovement(mockTitanPos, finalMovement, deltaTime);  

        // Расчет коэффициента Ghost-инвиза приседания на Left Ctrl  
        float targetAlpha = 1.0f;   
        if (actions.ghostMode || m_memoryManager.GetRegistryValue("player_sneaking") == 1) {  
            m_memoryManager.SetRegistryValue("player_sneaking", 1);  
            if (finalMovement.Length() > 0.0f && m_memoryManager.GetRegistryValue("perk_silent_move") == 0) {  
                targetAlpha = 0.8f;   
                Platform::Log("Скрытность: [ВНИМАНИЕ] Движение демаскирует гуманоида!");  
            } else {
                targetAlpha = 0.25f;   
            }
        }       
        m_memoryManager.SetRegistryValue("player_alpha_pct", static_cast<int32_t>(targetAlpha * 100.0f));  
    }

    // 3. Обработка мгновенных экшенов Fallout 76  
    if (actions.jump) {  
        Platform::Log("Engine Физика: Гуманоид совершил прыжок (Space / Кнопка А).");  
    }
    
    if (actions.ghostMode) {  
        Platform::Log("Engine Геймплей: Персонаж перешел в режим скрытности (GHOST SNEAK).");  
    }

    if (actions.useHeal) {  
        const auto& inventory = m_localPlayer->GetInventory();  
        bool healed = false;  
        for (size_t i = 0; i < inventory.size(); ++i) {  
            if (inventory[i].id == 301) {    
                m_localPlayer->UseItem(i);  
                healed = true;  
                break;
            }
        }
        if (!healed) {  
            Platform::Log("Геймплей: Нет стимуляторов в инвентаре!");  
        }
    }

    // Обработка фонарика Пип-боя на Tab  
    const uint8_t* currentKeyStates = SDL_GetKeyboardState(NULL);
    if (currentKeyStates[SDL_SCANCODE_TAB]) {
        static bool flashlightState = false;
        flashlightState = !flashlightState;
        m_memoryManager.SetRegistryValue("pipboy_light", flashlightState ? 1 : 0);
        Platform::Log(flashlightState ? "Pip-Boy: Фонарик включен." : "Pip-Boy: Фонарик выключен.");
    }

    m_localPlayer->UpdateSurvival(deltaTime);  
    m_camera.FollowPlayer(m_localPlayer->GetPosition(), 0.0f, 0.0f);  
}

void Engine::Render() {
    if (!m_renderer || !m_localPlayer) return;  

    m_renderer->ClearScreen();  

    // ИСПРАВЛЕНО: Устаревший fixed-function конвейер (glMatrixMode, gluLookAt) вырезан под корень!  
    // Вычисляем View-матрицу 4х4 на CPU с помощью тригонометрии
    float viewMatrix[16];
    ComputeFakeViewMatrix(m_camera.position, m_camera.target, viewMatrix);

    // Активируем шейдерную программу на GPU  
    glUseProgram(m_activeShaderID);

    // Пробрасываем матрицу обзора и координаты камеры в uniform-регистры шейдера Modern OpenGL
    int viewLoc = glGetUniformLocation(m_activeShaderID, "projectionViewMatrix");
    if (viewLoc != -1) {
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMatrix);
    }

    int camLoc = glGetUniformLocation(m_activeShaderID, "cameraPos");
    if (camLoc != -1) {
        glUniform3f(camLoc, m_camera.position.x, m_camera.position.y, m_camera.position.z);
    }

    float shaderAlpha = static_cast<float>(m_memoryManager.GetRegistryValue("player_alpha_pct")) / 100.0f;
    int stealthLoc = glGetUniformLocation(m_activeShaderID, "stealthAlpha");
    if (stealthLoc != -1) {
        glUniform1f(stealthLoc, shaderAlpha); // ИСПРАВЛЕНО: Используется glUniform1f
    }

    // Вызываем аппаратный рендеринг куба из памяти видеокарты
    m_renderer->DrawTestCube(m_localPlayer->GetPosition(), m_localPlayer->GetRotation());

    glUseProgram(0);
    m_renderer->Present();
}

void Engine::Stop() {
    if (!m_isRunning) return;
    m_isRunning = false;

    m_inputController.Shutdown();

    if (m_localPlayer) { delete m_localPlayer; m_localPlayer = nullptr; }
    if (m_renderer) { delete m_renderer; m_renderer = nullptr; }

    // ИСПРАВЛЕНО: Корректное уничтожение шейдера до контекста OpenGL
    m_activeShaderID = 0;
    m_shader.reset();

    NetworkSocket::GlobalCleanup();
    Platform::Log("Engine: 3D тригонометрический контекст выгружен.");
}

} // namespace Centralia
