#include <iostream>
#include <chrono>
#include <thread>

#define _USE_MATH_DEFINES // Гарантирует видимость M_PI в Math3D.hpp при сборке
#include <cmath>
#include "core/Engine.hpp"

// Твои базовые зависимости ядра Modern OpenGL 3.3 Core
#include "core/Math3D.hpp"
#include "gameplay/Player.hpp"
#include "gameplay/MonsterAISystem.hpp"
#include "gameplay/ProceduralMotionManager.hpp"
#include "platform/Platform.hpp"

// Пространство имен проекта по техническому паспорту
namespace Centralia {

// Инициализация глобальных контекстов игровых систем движка
static Player                   g_LocalPlayer;
static MonsterAISystem          g_MonsterAI;
static ProceduralMotionManager  g_TitanMotionManager;

/**
 * @brief Главная точка входа Windows 10 / Linux адаптации.
 * Содержит аппаратно-адаптивный цикл балансировки кадра движка.
 */
int main(int argc, char* argv[]) {
    Platform::Log("[CORE INIT]: Запуск Game Centralia: Dead Lend. Modern OpenGL 3.3 Core контекст активен.");

    // Инициализируем стартовых тактических ИИ-агентов на сцене (Одиночки и Рой роботов)
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

        // Квант времени deltaTime для точной симуляции физики и ИИ на CPU
        float deltaTime = static_cast<float>(elapsedTime.count());

        // Предохранитель на случай резкого падения FPS (например, при подгрузке модов .oggx)
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        // 1. Опрос контроллера ввода (Считывание WASD, Shift, Space, Ctrl, Tab)
        bool isShiftPressed = Platform::IsKeyPressed(Platform::KeyCode::Shift); // Спринт
        bool isCtrlPressed  = Platform::IsKeyPressed(Platform::KeyCode::Ctrl);  // Ghost-присед
        
        // Симулируем перемещение игрока на основе ввода в текущем кадре
        g_LocalPlayer.UpdateMovementState(deltaTime, isShiftPressed, isCtrlPressed);

        // 2. ИНТЕГРАЦИЯ ПУНКТА №4: Высокоуровневый обсчет скриптов псевдо-ИИ детекции монстров
        // Передаем стейты игрока для пассивного глушения шума в приседе сервоприводами силовой брони
        g_MonsterAI.ProcessAIScriptsTick(
            deltaTime, 
            g_LocalPlayer, 
            isShiftPressed && g_LocalPlayer.IsMoving(), 
            isCtrlPressed
        );

        // Пример отвлечения броском гильзы: если нажата кнопка 'G' (раскладка Fallout 76)
        if (Platform::IsKeyJustPressed(Platform::KeyCode::G)) {
            Vector3D casingTarget = { g_LocalPlayer.GetPosition().x + 12.0f, 51.0f, g_LocalPlayer.GetPosition().z + 4.0f };
            g_MonsterAI.ThrowWeaponCasingDistraction(casingTarget);
        }

        // 3. ИНТЕГРАЦИЯ РАЗДЕЛА II: Расчет ИИК-амортизации и пружин 4 лап Титана на CPU
        // Считываем высотный слой heightLevel карты из контекста Player (под его текущей позицией)
        float currentTileHeight = g_LocalPlayer.GetCurrentMapTileHeight(); 
        
        g_TitanMotionManager.UpdateTitanChassisIK(
            deltaTime, 
            g_LocalPlayer.GetPosition(), 
            currentTileHeight
        );

        // 4. Передача матриц трансформации, углов Roll/Pitch и позиций лап в Renderer3D
        float finalRoll   = g_TitanMotionManager.GetChassisRoll();
        float finalPitch  = g_TitanMotionManager.GetChassisPitch();
        // Рендерер Modern OpenGL применяет finalRoll и finalPitch к base_3d.vert шейдеру кадра

        // 5. АППАРАТНЫЙ БАЛАНСИРОВЩИК ХОСТА (Система "охлаждения" CPU/GPU под GTX 1050 Ti)
        auto frameEndTime = std::chrono::high_resolution_clock::now();
        auto frameDuration = frameEndTime - currentTime;

        if (frameDuration < targetFrameTime) {
            // Динамический сон CPU для предотвращения перегрева слабых встроенных систем
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

// Стандартная точка входа для линковщика компилятора
int main(int argc, char* argv[]) {
    return Centralia::main(argc, argv);
}
