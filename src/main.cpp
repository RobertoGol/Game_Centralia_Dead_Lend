#include "core/Engine.hpp"
#include "platform/Platform.hpp"
#include <SDL2/SDL.h>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    Centralia::Engine engine;

    if (!engine.Start()) {
        Centralia::Platform::Log("FATAL: Game engine failed to boot up context.");
        return -1;
    }

    SDL_Event event;
    
    // Переменные для контроля нагрузки и "охлаждения" железа
    auto targetFrameDuration = std::chrono::milliseconds(16); // Цель: ~60 FPS (16.6 мс на кадр)
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    
    float fpsUpdateTimer = 0.0f;
    int frameCount = 0;
    bool enableGpuCooling = false; // Режим защиты встройки / слабой GPU

    // Захватываем курсор мыши для 3D
    SDL_SetRelativeMouseMode(SDL_TRUE);

    // Главный аппаратно-адаптивный цикл Windows 10
    while (engine.IsRunning()) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        // Рассчитываем реальный deltaTime (время, за которое выполнился прошлый кадр)
        std::chrono::duration<float> elapsed = frameStart - lastFrameTime;
        lastFrameTime = frameStart;
        float deltaTime = elapsed.count();

        // Предохранитель от резких скачков (например, если окно фризануло)
        if (deltaTime > 0.1f) deltaTime = 0.1f; 

        // 1. ОПРОС СИСТЕМНЫХ СОБЫТИЙ (CPU)
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                engine.Stop();
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                engine.Stop();
            }
        }

        // 2. ОБСЧЕТ ЛОГИКИ (CPU) - Твой мощный проц щелкает это мгновенно
        // Физика движения WASD, крафт, сеть и тики выживания Fallout работают всегда плавно
        engine.Update();

        // 3. АДАПТИВНЫЙ РЕНДЕР (GPU)
        if (!enableGpuCooling || (frameCount % 2 == 0)) {
            // Если включено "охлаждение" для 1050Ti/встройки, мы рендерим геометрию через кадр (30 FPS),
            // чтобы освободить GPU от перегрева, но игра на CPU продолжает лететь на честных 60Гц!
            engine.Render();
        }

        // Подсчет производительности для автоматической балансировки
        frameCount++;
        fpsUpdateTimer += deltaTime;
        if (fpsUpdateTimer >= 1.0f) {
            // Если за секунду встройка выдала мало кадров, включаем режим разгрузки GPU
            if (frameCount < 45) { 
                if (!enableGpuCooling) {
                    enableGpuCooling = true;
                    Centralia::Platform::Log("[SYSTEM MONITOR]: GPU/In-build video bottleneck detected! Activating CPU-load balancer & Hardware Cooling Mode.");
                }
            } else if (frameCount > 55 && enableGpuCooling) {
                // Если железо остыло и FPS стабилизировался, возвращаем полную нагрузку
                enableGpuCooling = false;
                Centralia::Platform::Log("[SYSTEM MONITOR]: Hardware stabilized. Restoring maximum graphics pipeline.");
            }
            frameCount = 0;
            fpsUpdateTimer = 0.0f;
        }

        // 4. СИСТЕМА УМНОГО ОХЛАЖДЕНИЯ ПОТОКА (CPU + GPU Sleep)
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto frameProcessTime = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        
        if (frameProcessTime < targetFrameDuration) {
            // Замедляем поступление данных: принудительно усыпляем поток на остаток времени кадра.
            // Это разгружает процессор от холостого кручения цикла и дает видеокарте время закрыть очереди команд.
            std::this_thread::sleep_for(targetFrameDuration - frameProcessTime);
        } else {
            // Если железо жестко не справляется (время кадра > 16мс), даем микро-отдых системе на 1 миллисекунду,
            // чтобы операционная система Windows 10 / Arch успела обработать свои прерывания и сокеты
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    return 0;
}
