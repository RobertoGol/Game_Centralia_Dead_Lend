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

    // Фикс строки 26: Вызов переведен на валидный метод Initialize() согласно ItemDatabase.hpp [31]
    ItemDatabase::GetInstance().Initialize();
    CraftingManager::GetInstance().InitializeBlueprints();

    if (!NetworkSocket::GlobalInit()) return false;
    if (!m_memoryManager.Initialize(Platform::GetDeviceHWID())) return false;

    // 1. ПОДГРУЗКА ВЫСОКОПРОИЗВОДИТЕЛЬНОЙ 22-БАЙТОВОЙ КАРТЫ [31]
    MapSystem::GetInstance().LoadMapFromFile("test.map"); [31]

    // Запуск 3D-экрана Windows 10/Linux [31]
    m_renderer = new Renderer3D(1920, 1080); [31]
    if (!m_renderer->Initialize("Game Centralia: Dead Lend (Programmable GPU Core Build)")) { [31]
        return false;
    }

    // Компиляция шейдеров многоуровневых красок на GPU [31]
    Shader shaderCompiler;
    if (!shaderCompiler.LoadFromFiles("shaders/base_3d.vert", "shaders/base_3d.frag")) { [31]
        Platform::Log("Critical Error: GPU failed to compile core Centralia reflection shaders!"); [31]
        return false;
    }
    m_activeShaderID = shaderCompiler.GetProgramID();

    m_inputController.Initialize(); [31]

    m_localPlayer = new Player(777, "Vault_Survivor_76", 24); [31]
    m_localPlayer->SetPosition(Vector3D(0.0f, 0.0f, 0.0f)); [31]

    m_memoryManager.SetRegistryValue("player_alpha_pct", 100);  [31]

    // Фиксируем стартовую точку времени для аппаратно-адаптивного таймера
    m_lastFrameTime = std::chrono::high_resolution_clock::now();
    
    m_isRunning = true;
    Platform::Log("Engine: Core subsystems initialized. Programmable Hardware Pipeline linked."); [31]
    return true;
}

void Engine::HandleMouseMovement(float deltaX, float deltaY) {
    if (!m_localPlayer) return; [31]
    m_camera.FollowPlayer(m_localPlayer->GetPosition(), deltaX, deltaY); [31]
}

void Engine::Update() {
    if (!m_localPlayer) return; [31]

    // Рассчитываем честную дельту времени (deltaTime) между тиками процессора
    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentFrameTime - m_lastFrameTime).count();
    m_lastFrameTime = currentFrameTime;

    // Предохранитель от падения FPS (например, при зависании окна или брейкпоинте отладчика)
    if (deltaTime > 0.1f) deltaTime = 0.1f;

    // 1. Опрос геймпада/мыши для вращения 3D-камеры вокруг гуманоида [31]
    float gamepadLookX = 0.0f; [31]
    float gamepadLookY = 0.0f; [31]
    m_inputController.GetLookOffsets(gamepadLookX, gamepadLookY); [31]
    if (gamepadLookX != 0.0f || gamepadLookY != 0.0f) { [31]
        m_camera.FollowPlayer(m_localPlayer->GetPosition(), gamepadLookX, gamepadLookY); [31]
    }

    const GameplayActions& actions = m_inputController.GetActions(); [31]

    // 2. СИСТЕМА УПРАВЛЕНИЯ КЛАССАМИ (Админ-Хост против Обычного Пилота) [31]
    if (m_memoryManager.GetRegistryValue("active_control_mode") == static_cast<int32_t>(EngineControlMode::Admin_Observer)) { [31]
        m_camera.isAdminMode = true; [31]
        
        Vector3D inputDir = m_inputController.GetMovementVector(); [31]
        m_camera.MoveFreeCam(inputDir.z, inputDir.x, 0.0f, deltaTime); // ИСПРАВЛЕНО: Внедрена адаптивная дельта
        
        static uint32_t adminLogTick = 0; [31]
        if (adminLogTick++ % 300 == 0) { [31]
            Platform::Log("[ADMIN HOST]: Свободный полет админ-камеры активен. Мониторинг P2P-пакетов."); [31]
        }
    } 
    else {
        m_camera.isAdminMode = false;  [31]
        
        Vector3D inputDir = m_inputController.GetMovementVector(); [31]
        bool isMoving = (inputDir.Length() > 0.0f); [31]
        Vector3D finalMovement(0.0f, 0.0f, 0.0f); [31]

        if (isMoving) { [31]
            float currentSpeed = m_inputController.IsSprintPressed() ? 8.0f : 4.0f; [31]
            float angleRad = m_camera.yaw * static_cast<float>(M_PI) / 180.0f; [31]

            Vector3D cameraForward(std::sin(angleRad), 0.0f, std::cos(angleRad)); [31]
            Vector3D cameraRight(std::cos(angleRad), 0.0f, -std::sin(angleRad)); [31]

            finalMovement = (cameraForward * inputDir.z) + (cameraRight * inputDir.x); [31]
            
            // ВЫЧИСЛЕНИЕ ВЕКТОРНОЙ КОЛЛИЗИИ СТЕН НА CPU С УЧЕТОМ ДЕЛЬТЫ ВРЕМЕНИ [31]
            Vector3D predictedPosition = m_localPlayer->GetPosition() + (finalMovement.Normalize() * currentSpeed * deltaTime); [31]
            
            if (!MapSystem::GetInstance().CheckCollision(predictedPosition)) { [31]
                m_localPlayer->Move(finalMovement, currentSpeed, deltaTime); [31]
            } else {
                finalMovement = Vector3D(0.0f, 0.0f, 0.0f);  [31]
            }
            
            m_localPlayer->SetRotation(-m_camera.yaw - 90.0f); [31]
        }

        // Обсчет тиков дебаффов среды (Радиация, ЭМИ) на основе данных ячейки под ногами [31]
        // ИСПРАВЛЕНО: Строка 118 вылечена. Используем член класса m_classSystem вместо ежекадровой аллокации на стеке
        MapSystem::GetInstance().UpdateMapEnvironment(deltaTime, *m_localPlayer, m_classSystem); [31]

        // --- МАТЕМАТИЧЕСКИЙ ОБСЧЕТ ТИТАНА И ТЕХНИКИ (CPU) --- [31]
        static Vector3D mockTitanPos(5.0f, 0.0f, 5.0f); [31]
        if (finalMovement.Length() > 0.0f) { [31]
            mockTitanPos = mockTitanPos + (finalMovement.Normalize() * 3.5f * deltaTime); [31]
        }
        ProceduralMotionManager::GetInstance().UpdateTitanMovement(mockTitanPos, finalMovement, deltaTime); [31]

        // Расчет коэффициента Ghost-инвиза приседания на Left Ctrl [31]
        float targetAlpha = 1.0f;  [31]
        if (actions.ghostMode || m_memoryManager.GetRegistryValue("player_sneaking") == 1) { [31]
            m_memoryManager.SetRegistryValue("player_sneaking", 1); [31]
            if (finalMovement.Length() > 0.0f && m_memoryManager.GetRegistryValue("perk_silent_move") == 0) { [31]
                targetAlpha = 0.8f;  [31]
                Platform::Log("Скрытность: [ВНИМАНИЕ] Движение демаскирует гуманоида!"); [31]
            } else {
                targetAlpha = 0.25f;  [31]
            }
        }       
        m_memoryManager.SetRegistryValue("player_alpha_pct", static_cast<int32_t>(targetAlpha * 100.0f)); [31]
    }

    // 3. Обработка мгновенных экшенов Fallout 76 [31]
    if (actions.jump) { [31]
        Platform::Log("Engine Физика: Гуманоид совершил прыжок (Space / Кнопка А)."); [31]
    }
    
    if (actions.ghostMode) { [31]
        Platform::Log("Engine Геймплей: Персонаж перешел в режим скрытности (GHOST SNEAK)."); [31]
    }

    if (actions.useHeal) { [31]
        const auto& inventory = m_localPlayer->GetInventory(); [31]
        bool healed = false; [31]
        for (size_t i = 0; i < inventory.size(); ++i) { [31]
            if (inventory[i].id == 301) {   [31]
                m_localPlayer->UseItem(i); [31]
                healed = true; [31]
                break;
            }
        }
        if (!healed) { [31]
            Platform::Log("Геймплей: Нет стимуляторов в инвентаре!"); [31]
        }
    }

    // Обработка фонарика Пип-боя на Tab [31]
    const uint8_t* currentKeyStates = SDL_GetKeyboardState(NULL);
    if (currentKeyStates[SDL_SCANCODE_TAB]) {
        static bool flashlightState = false;
        flashlightState = !flashlightState;
        m_memoryManager.SetRegistryValue("pipboy_light", flashlightState ? 1 : 0);
        Platform::Log(flashlightState ? "Pip-Boy: Фонарик включен." : "Pip-Boy: Фонарик выключен.");
    }

    m_localPlayer->UpdateSurvival(deltaTime); [31]
    m_camera.FollowPlayer(m_localPlayer->GetPosition(), 0.0f, 0.0f); [31]
}

void Engine::Render() {
    if (!m_renderer || !m_localPlayer) return; [31]

    m_renderer->ClearScreen(); [31]

    // ИСПРАВЛЕНО: Устаревший fixed-function конвейер (glMatrixMode, gluLookAt) вырезан под корень! [31]
    // Вычисляем View-матрицу 4х4 на CPU с помощью тригонометрии
    float viewMatrix[16];
    ComputeFakeViewMatrix(m_camera.position, m_camera.target, viewMatrix);

    // Активируем шейдерную программу на GPU [31]
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
        glUniform3f(stealthLoc, shaderAlpha, 0.0f, 0.0f);
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

    if (m_activeShaderID != 0) {
        glDeleteProgram(m_activeShaderID);
        m_activeShaderID = 0;
    }

    NetworkSocket::GlobalCleanup();
    Platform::Log("Engine: 3D тригонометрический контекст выгружен.");
}

} // namespace Centralia
