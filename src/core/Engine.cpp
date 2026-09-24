#include "core/Engine.hpp"
#include "platform/Platform.hpp"
#include "gameplay/ProceduralMotion.hpp"
#include "core/NetworkSocket.hpp"
#include "core/NetworkProtocol.hpp"
#include "gameplay/ItemDatabase.hpp"
#include "gameplay/CraftingManager.hpp"
#include "gameplay/ModificationSystem.hpp"
#include <thread>
#include <chrono>
#include <cmath>

namespace Centralia {

Engine::Engine() : m_isRunning(false) {}
Engine::~Engine() { Stop(); }

bool Engine::Start() {
    if (!Platform::Initialize()) return false;

    // Инициализация глобальных геймплейных баз данных
    ItemDatabase::GetInstance().Initialize();
    CraftingManager::GetInstance().Initialize();

    if (!NetworkSocket::GlobalInit()) return false;
    if (!m_memoryManager.Initialize(Platform::GetDeviceHWID())) return false;

    // Запуск 3D-экрана Windows 10
    m_renderer = new Renderer3D(1920, 1080);
    if (!m_renderer->Initialize("Game Centralia: Dead Lend (Windows 10 Hardware Build)")) {
        return false;
    }

    // Загружаем и компилируем Vertex и Fragment шейдеры на видеокарту из папки shaders/
    if (!m_activeShader.LoadFromFiles("shaders/base_3d.vert", "shaders/base_3d.frag")) {
        Platform::Log("Critical Error: GPU failed to compile core Centralia reflection shaders!");
        return false;
    }

    // Инициализируем наш менеджер ввода (ищет геймпады Xbox/PS)
    m_inputController.Initialize();

    // Спавним нашего базового персонажа-гуманоида
    m_localPlayer = new Player(777, "Vault_Survivor_76", 24);
    m_localPlayer->SetPosition(Vector3D(0.0f, 0.0f, 0.0f));

    // По умолчанию выставляем альфа-канал видимости персонажа в реестр состояний
    m_memoryManager.SetRegistryValue("player_alpha_pct", 100); // 100% видимости

    m_isRunning = true;
    Platform::Log("Engine: Input-to-3D Camera тригонометрия успешно интегрирована в ядро.");
    return true;
}

void Engine::HandleMouseMovement(float deltaX, float deltaY) {
    if (!m_localPlayer) return;

    // Вращаем 3D-камеру вокруг гуманоида на основе дельты движения мыши
    m_camera.FollowPlayer(m_localPlayer->GetPosition(), deltaX, deltaY);
}

void Engine::Update() {
    if (!m_localPlayer) return;

    // 1. Опрос геймпада/мыши для вращения 3D-камеры вокруг гуманоида
    float gamepadLookX = 0.0f;
    float gamepadLookY = 0.0f;
    m_inputController.GetLookOffsets(gamepadLookX, gamepadLookY);
    if (gamepadLookX != 0.0f || gamepadLookY != 0.0f) {
        m_camera.FollowPlayer(m_localPlayer->GetPosition(), gamepadLookX, gamepadLookY);
    }

    // Получаем состояние команд управления Fallout 76
    const GameplayActions& actions = m_inputController.GetActions();

    // ПРОВЕРКА СКРЫТОГО КЛАССА АДМИНИСТРАТОРА (КОНСОЛЬНЫЙ ФИКСАТОР)
    if (m_memoryManager.GetRegistryValue("active_control_mode") == static_cast<int32_t>(ActiveControlMode::Admin_Observer)) {
        // Фиксатор хоста намертво блокирует позицию в координатах 0,0,0 (Середина карты)
        Vector3D adminCenterAnchor(0.0f, 0.0f, 0.0f);
        m_localPlayer->SetPosition(adminCenterAnchor);
        m_camera.target = adminCenterAnchor;
        
        static uint32_t adminLogTick = 0;
        if (adminLogTick++ % 300 == 0) {
            Platform::Log("[ADMIN HOST]: Центр карты зафиксирован. Мониторинг P2P-пакетов активен.");
        }
    } 
    else {
        // ОБЫЧНЫЙ ИГРОВОЙ РЕЖИМ (Обсчет движения WASD / Стика)
        Vector3D inputDir = m_inputController.GetMovementVector();
        bool isMoving = (inputDir.Length() > 0.0f);
        Vector3D finalMovement(0.0f, 0.0f, 0.0f);

        if (isMoving) {
            float currentSpeed = m_inputController.IsSprintPressed() ? 8.0f : 4.0f;
            float angleRad = m_camera.yaw * M_PI / 180.0f;

            Vector3D cameraForward(std::sin(angleRad), 0.0f, std::cos(angleRad));
            Vector3D cameraRight(std::cos(angleRad), 0.0f, -std::sin(angleRad));

            finalMovement = (cameraForward * inputDir.z) + (cameraRight * inputDir.x);
            
            m_localPlayer->Move(finalMovement, currentSpeed, 0.016f);
            m_localPlayer->SetRotation(-m_camera.yaw - 90.0f);
        }

        // --- МАТЕМАТИЧЕСКИЙ ОБСЧЕТ ТИТАНА И ТЕХНИКИ (CPU) ---
        static Vector3D mockTitanPos(5.0f, 0.0f, 5.0f);
        if (isMoving) {
            mockTitanPos = mockTitanPos + (finalMovement.Normalize() * 3.5f * 0.016f);
        }
        ProceduralMotionManager::GetInstance().UpdateTitanMovement(mockTitanPos, finalMovement, 0.016f);

        // Расчет коэффициента Ghost-инвиза приседания на Left Ctrl
        float targetAlpha = 1.0f; 
        if (actions.ghostMode || m_memoryManager.GetRegistryValue("player_sneaking") == 1) {
            m_memoryManager.SetRegistryValue("player_sneaking", 1);
            if (isMoving && m_memoryManager.GetRegistryValue("perk_silent_move") == 0) {
                targetAlpha = 0.8f; 
                Platform::Log("Скрытность: [ВНИМАНИЕ] Движение демаскирует гуманоида!");
            } else {
                targetAlpha = 0.25f; 
            }
        }       
        m_memoryManager.SetRegistryValue("player_alpha_pct", static_cast<int32_t>(targetAlpha * 100.0f));
    }

    // 3. Обработка мгновенных экшенов Fallout 76 (Прыжок, Скрытность, Быстрое лечение)
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
    const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
    if (currentKeyStates[SDL_SCANCODE_TAB]) {
        static bool flashlightState = false;
        flashlightState = !flashlightState;
        m_memoryManager.SetRegistryValue("pipboy_light", flashlightState ? 1 : 0);
        Platform::Log(flashlightState ? "Pip-Boy: Фонарик включен." : "Pip-Boy: Фонарик выключен.");
    }

    // 4. Обновляем тики показателей жизнедеятельности выжившего
    m_localPlayer->UpdateSurvival(0.016f);

    // 5. Камера от третьего лица следует за игроком
    m_camera.FollowPlayer(m_localPlayer->GetPosition(), 0.0f, 0.0f);
}

void Engine::Render() {
    if (!m_renderer || !m_localPlayer) return;

    m_renderer->ClearScreen();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    gluLookAt(
        m_camera.position.x, m_camera.position.y, m_camera.position.z,
        m_camera.target.x, m_camera.target.y + 1.0f, m_camera.target.z,
        0.0f, 1.0f, 0.0f
    );

    m_activeShader.Use();

    m_activeShader.SetVec3("cameraPos", m_camera.position.x, m_camera.position.y, m_camera.position.z);
    
    float shaderAlpha = static_cast<float>(m_memoryManager.GetRegistryValue("player_alpha_pct")) / 100.0f;
    m_activeShader.SetVec3("stealthAlpha", shaderAlpha, 0.0f, 0.0f); 

    m_renderer->DrawTestCube(m_localPlayer->GetPosition(), m_localPlayer->GetRotation());

    m_activeShader.Unuse();
    m_renderer->Present();
}

void Engine::Stop() {
    if (!m_isRunning) return;
    m_isRunning = false;

    m_inputController.Shutdown();

    if (m_localPlayer) { delete m_localPlayer; m_localPlayer = nullptr; }
    if (m_renderer) { delete m_renderer; m_renderer = nullptr; }

    NetworkSocket::GlobalCleanup();
    Platform::Log("Engine: 3D тригонометрический контекст выгружен.");
}

} // namespace Centralia
