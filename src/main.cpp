#define WIN32_LEAN_AND_MEAN // ИСПРАВЛЕНО: Блокирует старый winsock.h и лечит 100+ ошибок дублирования структур сети
#define _USE_MATH_DEFINES 
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

// ========================================================================
// CORE HEADER-ONLY PROTOCOL LAYER INCLUSIONS (Centralia Engine Passport)
// ========================================================================
#include "core/Engine.hpp"
#include "core/Math3D.hpp"
#include "core/MemoryManager.hpp"
#include "core/InputController.hpp"
#include "core/LoginSystem.hpp"
#include "core/ConfigSystem.hpp"
#include "core/AssetParser.hpp"

#include "gameplay/Player.hpp"
#include "gameplay/MonsterAISystem.hpp"
#include "gameplay/ProceduralMotionManager.hpp"
#include "gameplay/WeaponSystem.hpp"
#include "gameplay/FactorySystem.hpp"
#include "gameplay/DialogueSystem.hpp"
#include "gameplay/CreatureAI.hpp" // ИСПРАВЛЕНО: Путь синхронизирован с твоим реальным файлом на диске
#include "gameplay/ItemDatabase.hpp"
#include "gameplay/CraftingManager.hpp"
#include "gameplay/ServerShop.hpp"
#include "gameplay/VehiclePhysics.hpp"
#include "gameplay/PowerArmorStateData.hpp"

#include "video/Renderer3D.hpp"
#include "video/Shader.hpp"
#include "video/MaterialSystem.hpp"

#include "platform/Platform.hpp"
#include "platform/ResourcePackerOGGX.hpp"

namespace Centralia {

// Статические глобальные инстансы контекста симуляции
static Player          g_LocalPlayer;
static MonsterAISystem g_MonsterAI;

/**
 * @brief Главная точка входа Windows 10 / Linux адаптации.
 * Содержит аппаратно-адаптивный цикл балансировки кадра движка.
 */
int RunEngineMain(int argc, char* argv[]) {
    Platform::Log("[CORE INIT]: Запуск Game Centralia: Dead Lend. Modern OpenGL 3.3 Core контекст активен.");

    // Инициализация баз данных предметов и чертежей верстаков
    ItemDatabase::GetInstance().Initialize();
    CraftingManager::GetInstance().InitializeBlueprints();

    // Спавним стартовых тактических ИИ-агентов на сцене (Одиночки и Рой роботов)
    Vector3D individualSpawnPos = { 10.0f, 51.0f, 15.0f }; // Высотный слой 51 - Земля по техпаспорту
    Vector3D swarmSpawnPos      = { -20.0f, 51.0f, -5.0f };
    
    g_MonsterAI.SpawnTacticalAgent(101, "Behemoth_Alpha", AIArchetype::Individual, individualSpawnPos);
    g_MonsterAI.SpawnTacticalAgent(202, "Swarm_Drone_01", AIArchetype::SwarmDrone, swarmSpawnPos);
    g_MonsterAI.SpawnTacticalAgent(203, "Swarm_Drone_02", AIArchetype::SwarmDrone, swarmSpawnPos);

    // Целевое время кадра для жесткой балансировки на 60 FPS (~16.66 мс)
    constexpr std::chrono::duration<double, std::ratio<1, 60>> targetFrameTime(1);
    auto previousTime = std::chrono::high_resolution_clock::now();

    bool isEngineRunning = true;
    Platform::Log("[SYSTEM BALANCE]: Балансировщик CPU/GPU запущен. Целевая частота: 60 Гц.");

    // --- ОСНОВНОЙ ИГРОВОЙ ЦИКЛ ДВИЖКА (ENGINE TICK) ---
    while (isEngineRunning) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsedTime = currentTime - previousTime;
        previousTime = currentTime;

        float deltaTime = static_cast<float>(elapsedTime.count());

        // Предохранитель на случай резкого падения FPS (например, при подгрузке модов .oggx)
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // 1. Опрос контроллера ввода (Считывание WASD, Shift, Space, Ctrl, Tab)
        bool isShiftPressed = Platform::IsKeyPressed(KeyCode::Shift); // Спринт
        bool isCtrlPressed  = Platform::IsKeyPressed(KeyCode::Ctrl);  // Ghost-присед
        
        g_LocalPlayer.UpdateMovementState(deltaTime, isShiftPressed, isCtrlPressed);

        // 2. Высокоуровневый обсчет скриптов псевдо-ИИ детекции монстров
        g_MonsterAI.ProcessAIScriptsTick(
            deltaTime, 
            g_LocalPlayer, 
            isShiftPressed && g_LocalPlayer.IsMoving(), 
            isCtrlPressed
        );

        // Пример отвлечения броском гильзы: если нажата кнопка 'G' (раскладка Fallout 76)
        if (Platform::IsKeyJustPressed(KeyCode::G)) {
            Vector3D casingTarget = { g_LocalPlayer.GetPosition().x + 12.0f, 51.0f, g_LocalPlayer.GetPosition().z + 4.0f };
            g_MonsterAI.ThrowWeaponCasingDistraction(casingTarget);
        }

        // 3. ИСПРАВЛЕНО: Обновление ИИК-шасси Титана перенаправлено на валидный глобальный синглтон
        ProceduralMotionManager::GetInstance().UpdateTitanMovement(
            g_LocalPlayer.GetPosition(), 
            Vector3D(0.0f, 0.0f, 0.0f), 
            deltaTime
        );

        // 4. Передача матриц трансформации, углов Roll/Pitch в Renderer3D
        float finalRoll   = ProceduralMotionManager::GetInstance().GetChassisRoll();
        float finalPitch  = ProceduralMotionManager::GetInstance().GetChassisPitch();

        // 5. АППАРАТНЫЙ БАЛАНСИРОВЩИК ХОСТА (Система "охлаждения" CPU/GPU под GTX 1050 Ti)
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto frameDuration = frameEndTime - currentTime;

        if (frameDuration < targetFrameTime) {
            auto sleepTime = targetFrameTime - frameDuration;
            std::this_thread::sleep_for(std::chrono::duration_cast<std::chrono::milliseconds>(sleepTime));
        }

        // Условие выхода из игры (например, закрытие окна OpenGL)
        if (Platform::WindowShouldClose()) {
            isEngineRunning = false;
        }
    }

    Platform::Log("[CORE SHUTDOWN]: Игровой цикл завершен. Освобождение контекстов памяти.");
    return 0;
}

} // namespace Centralia

// Точка входа линковщика компилятора в глобальном пространстве
int main(int argc, char* argv[]) {
    return Centralia::RunEngineMain(argc, argv);
}
