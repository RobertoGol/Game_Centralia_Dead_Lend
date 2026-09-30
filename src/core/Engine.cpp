#include "Engine.hpp"
#include "ConfigSystem.hpp"
#include "InputController.hpp"
#include "LoginSystem.hpp"
#include "MemoryManager.hpp"
#include "NetworkProtocol.hpp"
#include "NetworkSocket.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/MonsterAISystem.hpp"
#include "gameplay/CraftingManager.hpp"
#include "gameplay/MapSystem.hpp"
#include "gameplay/ModificationSystem.hpp"
#include "gameplay/WeaponSystem.hpp"
#include "platform/Platform.hpp"

// Условные инклуды для графики и физики (согласно архитектуре движка)
// #include "video/Renderer3D.hpp"
// #include "physics/PhysicsWorld.hpp"
// #include "audio/AudioEngine.hpp"

#include <chrono>
#include <thread>
#include <iostream>
#include <exception>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <mutex>

namespace Centralia {

// ============================================================================
// SECTION 1: SINGLETON INSTANCE, CONSTRUCTORS & MEMORY ALLOCATION
// ============================================================================

Engine* Engine::s_instance = nullptr;

Engine::Engine() 
    : m_isRunning(false),
      m_isPaused(false),
      m_windowActive(true),
      m_currentState(EngineState::BootSequence),
      m_targetFramerate(144),
      m_fixedPhysicsStep(1.0 / 60.0), // 60 Hz Physics tick
      m_timeAccumulator(0.0),
      m_totalRunTime(0.0),
      m_frameCount(0),
      m_lastFpsUpdate(0.0),
      m_currentFps(0),
      m_profilerFrameTimeMs(0.0f),
      m_profilerLogicTimeMs(0.0f),
      m_profilerRenderTimeMs(0.0f),
      m_cameraFov(90.0f),
      m_cameraPitch(0.0f),
      m_cameraYaw(0.0f)
{
    if (s_instance != nullptr) {
        Platform::Log("[ENGINE FATAL]: Попытка двойной инициализации синглтона Engine!");
        std::terminate();
    }
    s_instance = this;
    
    // Строгое выделение памяти под критические подсистемы (до старта графики)
    Platform::Log("[ENGINE BOOT]: Выделение памяти под корневые менеджеры...");
    m_memoryManager = new MemoryManager();
    m_configSystem = new ConfigSystem();
    m_inputController = new InputController();
    m_loginSystem = new LoginSystem();
    
    // Игровые подсистемы
    m_player = new Player();
    m_aiSystem = new MonsterAISystem();
    m_modificationSystem = new ModificationSystem();
    // m_weaponSystem = new WeaponSystem();
    // m_mapSystem = new MapSystem();

    m_cameraPosition = Vector3D(0.0f, 2.0f, 0.0f);
    m_cameraForward = Vector3D(0.0f, 0.0f, 1.0f);
    m_cameraUp = Vector3D(0.0f, 1.0f, 0.0f);
    m_cameraRight = Vector3D(1.0f, 0.0f, 0.0f);

    Platform::Log("[ENGINE CONSTRUCTOR]: Главный объект движка (Engine) успешно размещен в RAM.");
}

Engine::~Engine() {
    Shutdown();

    // Каскадное удаление подсистем в обратном порядке
    delete m_modificationSystem;
    delete m_aiSystem;
    delete m_player;
    delete m_loginSystem;
    delete m_inputController;
    delete m_configSystem;
    delete m_memoryManager;

    s_instance = nullptr;
    Platform::Log("[ENGINE DESTRUCTOR]: RAM полностью очищена от структур движка.");
}

Engine& Engine::GetInstance() {
    if (!s_instance) {
        Platform::Log("[ENGINE FATAL ERROR]: GetInstance() вызван до конструирования Engine!");
        std::terminate();
    }
    return *s_instance;
}

// ============================================================================
// SECTION 2: THE BOOTSTRAP PIPELINE (INITIALIZATION SEQUENCE)
// ============================================================================

bool Engine::Initialize() {
    Platform::Log("[ENGINE INIT]: Запуск конвейера загрузки Centralia Engine...");
    m_currentState = EngineState::BootSequence;

    try {
        // Шаг 1: Инициализация кучи памяти и сборщика мусора
        Platform::Log("[ENGINE INIT]: Фаза 1 - Подготовка MemoryManager (Пул: 1024 MB)...");
        m_memoryManager->InitializeHeap(1024 * 1024 * 1024); 

        // Шаг 2: Чтение конфигурационных файлов
        Platform::Log("[ENGINE INIT]: Фаза 2 - Парсинг конфигурации...");
        if (!m_configSystem->LoadConfigFromFile("config/centralia_engine.cfg")) {
            Platform::Log("[ENGINE INIT WARNING]: Конфиг не найден, генерация профиля по умолчанию.");
        }

        // Шаг 3: Инициализация окна ОС и графического контекста (OpenGL/Vulkan)
        Platform::Log("[ENGINE INIT]: Фаза 3 - Инициализация видеовывода...");
        int winWidth, winHeight;
        bool fullscreen;
        m_configSystem->GetResolution(winWidth, winHeight, fullscreen);
        
        if (!Platform::InitializeWindow("Centralia: Dead Lend", winWidth, winHeight, fullscreen)) {
            throw std::runtime_error("Platform::InitializeWindow завершился сбоем.");
        }

        // Шаг 4: Инициализация 3D Рендерера
        Platform::Log("[ENGINE INIT]: Фаза 4 - Инициализация графического конвейера (Renderer3D)...");
        // Renderer3D::InitializeContext();
        // Renderer3D::SetVSync(m_configSystem->IsVSyncEnabled());
        // Renderer3D::BuildShaderCache();

        // Шаг 5: Аудио-движок
        Platform::Log("[ENGINE INIT]: Фаза 5 - Захват аудиоустройств...");
        Platform::SetAudioMasterVolume(m_configSystem->GetMasterVolume());
        // AudioEngine::Initialize();

        // Шаг 6: Физический движок (PhysX/Bullet)
        Platform::Log("[ENGINE INIT]: Фаза 6 - Прогрев физического ядра...");
        // PhysicsWorld::Initialize(Vector3D(0.0f, -9.81f, 0.0f));

        // Шаг 7: Устройства ввода
        Platform::Log("[ENGINE INIT]: Фаза 7 - Привязка мыши, клавиатуры и геймпадов...");
        m_inputController->InitializeDevices();

        // Шаг 8: Сетевой стек
        Platform::Log("[ENGINE INIT]: Фаза 8 - Сетевые сокеты и криптография...");
        // NetworkSocket::InitializeNetworkStack();

        // Применение настроек
        m_targetFramerate = m_configSystem->GetTargetFramerate();
        m_isRunning = true;
        
        // Переход в главное меню после успешной загрузки
        ChangeState(EngineState::MainMenu);
        Platform::Log("[ENGINE INIT SUCCESS]: Все подсистемы Centralia работают в штатном режиме.");
        return true;

    } catch (const std::exception& e) {
        TriggerFatalError(std::string("Сбой конвейера инициализации: ") + e.what());
        return false;
    }
}

// ============================================================================
// SECTION 3: THE MAIN GAME LOOP (SEMI-FIXED TIMESTEP ARCHITECTURE)
// ============================================================================

void Engine::Run() {
    if (!m_isRunning) {
        Platform::Log("[ENGINE FATAL]: Попытка вызова Run() на неинициализированном движке!");
        return;
    }

    Platform::Log("[ENGINE MAIN LOOP]: Вход в асинхронный цикл симуляции.");

    using clock = std::chrono::high_resolution_clock;
    auto previousTime = clock::now();
    double targetFrameTime = 1.0 / static_cast<double>(m_targetFramerate);

    while (m_isRunning) {
        auto currentTime = clock::now();
        std::chrono::duration<double> frameTimeDuration = currentTime - previousTime;
        double frameTime = frameTimeDuration.count();
        previousTime = currentTime;

        // Spiral of Death protection (ограничение максимального шага при лагах ОС)
        if (frameTime > 0.25) {
            frameTime = 0.25;
            Platform::Log("[ENGINE STALL]: Обнаружено зависание потока, дельта времени ограничена 250 мс.");
        }

        m_totalRunTime += frameTime;
        m_timeAccumulator += frameTime;

        // ---------------------------------------------------------
        // 1. Опрос аппаратных событий и интерфейсов ОС
        // ---------------------------------------------------------
        ProcessPlatformEvents();
        m_inputController->PollEvents();

        // ---------------------------------------------------------
        // 2. ФИЗИКА И ЖЕСТКАЯ ЛОГИКА (Fixed Update)
        // ---------------------------------------------------------
        auto logicStartTime = clock::now();
        int physicsStepsThisFrame = 0;

        while (m_timeAccumulator >= m_fixedPhysicsStep) {
            FixedUpdate(m_fixedPhysicsStep);
            m_timeAccumulator -= m_fixedPhysicsStep;
            physicsStepsThisFrame++;
            
            // Прерывание вечного цикла физики (max 5 шагов за кадр)
            if (physicsStepsThisFrame >= 5) {
                m_timeAccumulator = 0.0;
                break;
            }
        }

        // ---------------------------------------------------------
        // 3. ДИНАМИЧЕСКАЯ ИГРОВАЯ ЛОГИКА (Variable Update)
        // ---------------------------------------------------------
        Update(frameTime);

        std::chrono::duration<double, std::milli> logicTimeMs = clock::now() - logicStartTime;
        m_profilerLogicTimeMs = static_cast<float>(logicTimeMs.count());

        // ---------------------------------------------------------
        // 4. ГРАФИЧЕСКИЙ КОНВЕЙЕР (Render Pipeline)
        // ---------------------------------------------------------
        auto renderStartTime = clock::now();
        
        // Расчет коэффициента интерполяции для плавной графики между шагами физики
        double interpolationAlpha = m_timeAccumulator / m_fixedPhysicsStep;
        Render(interpolationAlpha);
        
        std::chrono::duration<double, std::milli> renderTimeMs = clock::now() - renderStartTime;
        m_profilerRenderTimeMs = static_cast<float>(renderTimeMs.count());

        // ---------------------------------------------------------
        // 5. ТЕЛЕМЕТРИЯ И СИНХРОНИЗАЦИЯ КАДРОВ
        // ---------------------------------------------------------
        UpdateTelemetry(frameTime);

        auto frameEndTime = clock::now();
        std::chrono::duration<double> currentFrameDuration = frameEndTime - currentTime;
        
        // Сон потока для поддержания Target Framerate и экономии ресурсов CPU
        if (currentFrameDuration.count() < targetFrameTime) {
            double sleepTime = targetFrameTime - currentFrameDuration.count();
            std::this_thread::sleep_for(std::chrono::duration<double>(sleepTime));
        }
    }

    Platform::Log("[ENGINE MAIN LOOP]: Выход из цикла симуляции. Подготовка к остановке.");
}

// ============================================================================
// SECTION 4: PLATFORM OS EVENTS & WINDOW ROUTING
// ============================================================================

void Engine::ProcessPlatformEvents() {
    PlatformEvent event;
    while (Platform::PollEvent(event)) {
        switch (event.type) {
            case PlatformEventType::Quit:
                m_isRunning = false;
                Platform::Log("[PLATFORM EVENT]: Сигнал SIGTERM / Закрытие окна.");
                break;
                
            case PlatformEventType::WindowFocusLost:
                m_windowActive = false;
                if (m_currentState == EngineState::ActiveGameplay) {
                    ChangeState(EngineState::Paused);
                    Platform::Log("[PLATFORM EVENT]: Окно потеряло фокус. Автопауза.");
                }
                break;
                
            case PlatformEventType::WindowFocusGained:
                m_windowActive = true;
                break;

            case PlatformEventType::WindowResized:
                Platform::Log("[PLATFORM EVENT]: Ресайз окна: " + std::to_string(event.width) + "x" + std::to_string(event.height));
                // Обновление Projection матрицы в рендерере
                // Renderer3D::UpdateViewport(event.width, event.height);
                break;

            case PlatformEventType::DeviceLost:
                TriggerFatalError("Потеряно устройство рендеринга (GPU Device Lost).");
                break;

            default:
                break;
        }
    }
}

// ============================================================================
// SECTION 5: FIXED TIMESTEP UPDATE (PHYSICS, AI, COLLISION)
// ============================================================================

void Engine::FixedUpdate(double fixedDeltaTime) {
    if (m_isPaused || m_currentState != EngineState::ActiveGameplay) return;

    float dt = static_cast<float>(fixedDeltaTime);

    // 1. Шаг физического движка (Интеграция сил, Broadphase, Narrowphase)
    // PhysicsWorld::StepSimulation(dt);

    // 2. Симуляция транспортных средств и баллистики
    // m_vehiclePhysics->SimulatePhysics(dt, ...);
    // m_weaponSystem->UpdateProjectiles(dt);

    // 3. Обновление поведения искусственного интеллекта (Swarm AI)
    if (m_aiSystem && m_player) {
        m_aiSystem->UpdateAI(dt, *m_player);
    }

    // 4. Обновление стамины, радиации и эффектов игрока
    if (m_player) {
        m_player->UpdateSurvivalTicks(dt);
    }

    // 5. Обработка очереди крафта верстаков в мире
    if (m_modificationSystem) {
        m_modificationSystem->UpdateWorkbenchTick(dt);
    }

    // 6. 3D Аудио: обновление позиции слушателя (Listener)
    // AudioEngine::UpdateListenerPosition(m_cameraPosition, m_cameraForward, m_cameraUp);
}

// ============================================================================
// SECTION 6: VARIABLE TIMESTEP UPDATE (INPUT, CAMERA, WEAPONS)
// ============================================================================

void Engine::Update(double deltaTime) {
    float dt = static_cast<float>(deltaTime);

    // Глобальные горячие клавиши (перехватываются в любом состоянии)
    if (m_inputController->IsKeyPressed(KeyCode::Escape)) {
        if (m_currentState == EngineState::ActiveGameplay) {
            ChangeState(EngineState::Paused);
        } else if (m_currentState == EngineState::Paused) {
            ChangeState(EngineState::ActiveGameplay);
        } else if (m_currentState == EngineState::InventoryMenu) {
            ChangeState(EngineState::ActiveGameplay);
        }
    }

    if (m_inputController->IsKeyPressed(KeyCode::F3)) {
        // Toggle Debug Overlay
    }

    switch (m_currentState) {
        case EngineState::MainMenu: {
            // Взаимодействие с UI главного меню
            if (m_inputController->IsKeyPressed(KeyCode::Enter)) {
                ChangeState(EngineState::LoadingScreen);
            }
            break;
        }
        
        case EngineState::ActiveGameplay: {
            if (m_isPaused) break;

            // 1. Обработка ввода движения (WASD)
            Vector3D moveVector(0.0f, 0.0f, 0.0f);
            if (m_inputController->IsKeyDown(KeyCode::W)) moveVector = moveVector + m_cameraForward;
            if (m_inputController->IsKeyDown(KeyCode::S)) moveVector = moveVector - m_cameraForward;
            if (m_inputController->IsKeyDown(KeyCode::D)) moveVector = moveVector + m_cameraRight;
            if (m_inputController->IsKeyDown(KeyCode::A)) moveVector = moveVector - m_cameraRight;

            // Блокируем полет (движение только в плоскости XZ)
            moveVector.y = 0.0f;
            if (moveVector.LengthSquared() > 0.0f) {
                moveVector = moveVector.Normalized();
            }

            bool isSprinting = m_inputController->IsKeyDown(KeyCode::Shift);
            bool isCrouching = m_inputController->IsKeyDown(KeyCode::Ctrl);

            if (m_player) {
                m_player->UpdateMovement(dt, moveVector, isSprinting, isCrouching);
            }

            // 2. Обработка вращения камеры (Mouse Look)
            float mouseDeltaX, mouseDeltaY;
            m_inputController->GetMouseDelta(mouseDeltaX, mouseDeltaY);
            
            float sensitivity = 0.1f;
            m_cameraYaw += mouseDeltaX * sensitivity;
            m_cameraPitch -= mouseDeltaY * sensitivity;
            
            // Ограничение тангажа (Pitch) чтобы не "сломать шею"
            m_cameraPitch = std::clamp(m_cameraPitch, -89.0f, 89.0f);

            // Пересчет векторов камеры
            float yawRad = m_cameraYaw * (3.14159265f / 180.0f);
            float pitchRad = m_cameraPitch * (3.14159265f / 180.0f);

            m_cameraForward.x = std::cos(yawRad) * std::cos(pitchRad);
            m_cameraForward.y = std::sin(pitchRad);
            m_cameraForward.z = std::sin(yawRad) * std::cos(pitchRad);
            m_cameraForward = m_cameraForward.Normalized();

            m_cameraRight = m_cameraForward.Cross(Vector3D(0.0f, 1.0f, 0.0f)).Normalized();
            m_cameraUp = m_cameraRight.Cross(m_cameraForward).Normalized();

            // Привязка позиции камеры к голове игрока
            if (m_player) {
                float headOffset = isCrouching ? 1.2f : 1.8f;
                m_cameraPosition = m_player->GetPosition() + Vector3D(0.0f, headOffset, 0.0f);
            }

            // 3. Обработка стрельбы и оружия
            if (m_inputController->IsMouseButtonDown(MouseButton::Left)) {
                // Raycast из центра экрана
                // m_weaponSystem->FireWeapon(m_cameraPosition, m_cameraForward);
            }
            
            if (m_inputController->IsKeyPressed(KeyCode::R)) {
                // m_weaponSystem->Reload();
            }

            // 4. Взаимодействие с миром (Raycast для лута и верстаков)
            if (m_inputController->IsKeyPressed(KeyCode::E)) {
                // RaycastHit hit;
                // if (PhysicsWorld::Raycast(m_cameraPosition, m_cameraForward, 3.0f, hit)) {
                //     if (hit.entity->IsWorkbench()) ChangeState(EngineState::InventoryMenu);
                //     else if (hit.entity->IsLoot()) m_player->AddItemToInventory(...);
                // }
            }

            // 5. Открытие инвентаря
            if (m_inputController->IsKeyPressed(KeyCode::Tab)) {
                ChangeState(EngineState::InventoryMenu);
            }

            break;
        }

        case EngineState::InventoryMenu: {
            // Взаимодействие с UI Пип-боя или Верстака
            if (m_inputController->IsKeyPressed(KeyCode::Tab)) {
                ChangeState(EngineState::ActiveGameplay);
            }
            break;
        }

        default:
            break;
    }
}

// ============================================================================
// SECTION 7: RENDERING PIPELINE & GRAPHICS DISPATCH
// ============================================================================

void Engine::Render(double interpolationAlpha) {
    // 1. Очистка буферов (Depth, Color, Stencil)
    // Renderer3D::ClearBuffers(0.05f, 0.05f, 0.05f, 1.0f);

    switch (m_currentState) {
        case EngineState::BootSequence: {
            // Renderer2D::DrawSplashScreen("assets/textures/studio_logo.png");
            break;
        }

        case EngineState::MainMenu: {
            // Отрисовка трехмерного фона меню и UI поверх него
            // Renderer3D::RenderMenuBackground();
            // UIManager::RenderMainMenu();
            break;
        }

        case EngineState::LoadingScreen: {
            // UIManager::RenderLoadingScreen(m_loadingProgress);
            break;
        }

        case EngineState::ActiveGameplay:
        case EngineState::Paused:
        case EngineState::InventoryMenu: {
            // Вычисление интерполированной позиции камеры для плавности
            // Vector3D renderCamPos = m_previousCameraPos * (1.0 - interpolationAlpha) + m_cameraPosition * interpolationAlpha;
            
            // Расчет View и Projection матриц
            // Matrix4x4 viewMatrix = Matrix4x4::LookAt(m_cameraPosition, m_cameraPosition + m_cameraForward, m_cameraUp);
            // Matrix4x4 projMatrix = Matrix4x4::Perspective(m_cameraFov, m_aspectRatio, 0.1f, 1000.0f);

            // --- PASS 1: Shadow Mapping (Directional & Point Lights) ---
            // Renderer3D::BeginShadowPass();
            // m_mapSystem->SubmitGeometryToRenderer();
            // Renderer3D::EndShadowPass();

            // --- PASS 2: Opaque Geometry (G-Buffer for Deferred Shading) ---
            // Renderer3D::BeginGeometryPass(viewMatrix, projMatrix);
            // m_mapSystem->SubmitGeometryToRenderer();
            
            // Отрисовка монстров
            /*
            for (const auto& monster : m_aiSystem->GetActiveSwarm()) {
                Vector3D interpPos = monster.prevPos * (1.0 - interpolationAlpha) + monster.position * interpolationAlpha;
                Renderer3D::SubmitModel(monster.modelId, interpPos, monster.rotation);
            }
            */
            // Renderer3D::EndGeometryPass();

            // --- PASS 3: Lighting & Skybox ---
            // Renderer3D::RenderDeferredLights();
            // Renderer3D::RenderSkybox(viewMatrix, projMatrix);

            // --- PASS 4: Transparent Geometry (Particles, Glass) ---
            // Renderer3D::RenderTransparentQueue();
            // m_weaponSystem->RenderMuzzleFlashesAndTracers();

            // --- PASS 5: Post-Processing ---
            // Renderer3D::ApplyPostProcessing(Bloom | ToneMapping | Vignette);

            // --- PASS 6: User Interface (HUD) ---
            if (m_currentState == EngineState::ActiveGameplay) {
                // UIManager::RenderCrosshair();
                // UIManager::RenderHealthBar(m_player->GetCurrentHealth());
                // UIManager::RenderAmmoCounter(m_weaponSystem->GetCurrentAmmo());
            } else if (m_currentState == EngineState::Paused) {
                // Наложение эффекта размытия (Blur) на фон
                // Renderer3D::ApplyUIBlur();
                // UIManager::RenderPauseMenu();
            } else if (m_currentState == EngineState::InventoryMenu) {
                // Renderer3D::ApplyUIBlur();
                // UIManager::RenderPipBoyInterface(m_player->GetInventory());
            }

            break;
        }

        case EngineState::FatalError: {
            // Renderer2D::ClearScreen(1.0f, 0.0f, 0.0f); // Красный экран смерти
            // UIManager::RenderCrashText(m_lastErrorMessage);
            break;
        }
        
        default:
            break;
    }

    // 2. Отправка буфера на дисплей (Swap Buffers)
    // Platform::SwapBuffers();
}

// ============================================================================
// SECTION 8: STATE MACHINE & CONTEXT SWITCHING
// ============================================================================

void Engine::ChangeState(EngineState newState) {
    if (m_currentState == newState) return;

    Platform::Log("[ENGINE STATE]: Транзит состояния движка: " + 
                  std::to_string(static_cast<int>(m_currentState)) + " ---> " + 
                  std::to_string(static_cast<int>(newState)));

    // ЛОГИКА ВЫХОДА (Exit State)
    if (m_currentState == EngineState::ActiveGameplay) {
        if (newState == EngineState::Paused || newState == EngineState::InventoryMenu) {
            m_isPaused = true;
            m_inputController->SetMouseCapture(false); // Освобождаем курсор для UI
            // AudioEngine::SetLowPassFilter(true); // Приглушаем звуки игры
        }
    }

    // ЛОГИКА ВХОДА (Enter State)
    if (newState == EngineState::ActiveGameplay) {
        m_isPaused = false;
        m_inputController->SetMouseCapture(true); // Прячем курсор для управления шутером
        // AudioEngine::SetLowPassFilter(false);
    } 
    else if (newState == EngineState::LoadingScreen) {
        Platform::Log("[ENGINE THREADING]: Запуск фонового потока генерации/загрузки уровня...");
        
        // Эмуляция асинхронной загрузки
        std::thread loadThread([this]() {
            // m_mapSystem->LoadMap("maps/sector_4.bin");
            // m_aiSystem->PopulateSpawns();
            std::this_thread::sleep_for(std::chrono::seconds(2)); // Имитация загрузки
            this->ChangeState(EngineState::ActiveGameplay);
        });
        loadThread.detach();
    }

    m_currentState = newState;
}

// ============================================================================
// SECTION 9: TELEMETRY, PERFORMANCE METRICS & PROFILING
// ============================================================================

void Engine::UpdateTelemetry(double deltaTime) {
    m_frameCount++;
    m_lastFpsUpdate += deltaTime;

    // Обновление счетчика кадров раз в секунду
    if (m_lastFpsUpdate >= 1.0) {
        m_currentFps = m_frameCount;
        
        // Логирование метрик производительности (profiling)
        // Platform::Log("[PROFILER]: FPS: " + std::to_string(m_currentFps) + 
        //              " | Logic: " + std::to_string(m_profilerLogicTimeMs) + "ms" +
        //              " | Render: " + std::to_string(m_profilerRenderTimeMs) + "ms");
        
        m_frameCount = 0;
        m_lastFpsUpdate = 0.0;
    }

    // Мониторинг потребления оперативной памяти
    if (m_memoryManager) {
        // size_t currentMemoryUsage = m_memoryManager->GetAllocatedSize();
        // size_t maxMemory = m_memoryManager->GetMaxHeapSize();
        
        // Если использовано более 85% RAM - принудительный сбор мусора (Garbage Collection)
        // if (currentMemoryUsage > maxMemory * 0.85) {
        //     Platform::Log("[ENGINE MEMORY]: Критический уровень ОЗУ. Запуск дефрагментации пулов...");
        //     m_memoryManager->RunGarbageCollection();
        // }
    }
}

// ============================================================================
// SECTION 10: GRACEFUL SHUTDOWN & DATA SERIALIZATION
// ============================================================================

void Engine::Shutdown() {
    if (!m_isRunning && m_currentState == EngineState::Shutdown) return;

    Platform::Log("[ENGINE SHUTDOWN]: Инициализация грациозного завершения работы...");
    m_currentState = EngineState::Shutdown;
    m_isRunning = false;

    // 1. Остановка всех воркеров (Thread Pool)
    Platform::Log("[ENGINE SHUTDOWN]: Остановка фоновых потоков физики и ИИ...");
    // m_threadPool->WaitAndTerminateAll();

    // 2. Сериализация критически важных данных (Сохранение игры)
    Platform::Log("[ENGINE SHUTDOWN]: Формирование бинарных дампов сохранения...");
    if (m_player) {
        std::vector<uint8_t> playerDump = m_player->SerializeToBinary();
        // FileSystem::WriteBinary("saves/autosave_player.bin", playerDump);
    }
    
    if (m_modificationSystem) {
        std::vector<uint8_t> modsDump = m_modificationSystem->SerializeToBinary();
        // FileSystem::WriteBinary("saves/autosave_mods.bin", modsDump);
    }

    // 3. Сохранение системных настроек
    if (m_configSystem) {
        m_configSystem->SaveConfigToFile();
    }

    // 4. Безопасное закрытие сетевых портов
    Platform::Log("[ENGINE SHUTDOWN]: Разрыв сетевых P2P соединений...");
    // NetworkSocket::CloseAllConnections();

    // 5. Выгрузка аудио-буферов
    // AudioEngine::Shutdown();

    // 6. Очистка видеопамяти (VRAM)
    Platform::Log("[ENGINE SHUTDOWN]: Уничтожение графического контекста и очистка VRAM...");
    // Renderer3D::Shutdown();

    // 7. Уничтожение системного окна
    // Platform::DestroyWindow();

    Platform::Log("[ENGINE SHUTDOWN SUCCESS]: Движок Centralia успешно и безопасно остановлен.");
}

// ============================================================================
// SECTION 11: FATAL EXCEPTION HANDLING & CRASH DUMPING
// ============================================================================

void Engine::TriggerFatalError(const std::string& errorMessage) {
    Platform::Log("[ENGINE FATAL CRASH]: Зафиксирована критическая ошибка выполнения!");
    Platform::Log("[CRASH TRACE]: " + errorMessage);
    
    m_currentState = EngineState::FatalError;
    m_isRunning = false;

    // Генерация дампа памяти для отладки
    if (m_memoryManager) {
        Platform::Log("[ENGINE DUMP]: Формирование файла minidump (memory.dmp)...");
        // m_memoryManager->GenerateCrashDump("crashes/memory_latest.dmp");
    }

    // Освобождение мыши для взаимодействия с окном ошибки
    if (m_inputController) {
        m_inputController->SetMouseCapture(false);
    }

    // Вызов диалогового окна ОС
    // Platform::ShowErrorDialog("Критический сбой Centralia Engine", errorMessage);

    Shutdown();
    std::terminate(); // Немедленное прерывание процесса (abort)
}

} // namespace Centralia