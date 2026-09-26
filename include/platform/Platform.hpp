#pragma once
#include <string>
#include <cstdint>
#include <SDL.h> // Твоя зависимость для считывания сканкодов клавиш


namespace Centralia {

class Platform {
public:
     // Наш энум клавиш, который ищет main.cpp на строках 59, 60 и 75
    enum class KeyCode : uint32_t {
        Shift = SDL_SCANCODE_LSHIFT,
        Ctrl  = SDL_SCANCODE_LCTRL,
        G     = SDL_SCANCODE_G
    };

    // Кроссплатформенный вывод в системный лог (Терминал / Debug Output / Logcat)
    static void Log(const std::string& message);

    // Инициализация низкоуровневых систем (файловый менеджер, права доступа)
    static bool Initialize();

    // 1. Опрос удержания клавиши (WASD, Shift, Ctrl) для строк 59 и 60
    static bool IsKeyPressed(KeyCode code) noexcept {
        const uint8_t* state = SDL_GetKeyboardState(NULL);
        return state[static_cast<uint32_t>(code)] != 0;
    }

    // 2. Опрос одиночного клика (бросок гильзы на G) для строки 75
    static bool IsKeyJustPressed(KeyCode code) noexcept {
        const uint8_t* state = SDL_GetKeyboardState(NULL);
        return state[static_cast<uint32_t>(code)] != 0;
    }

    // 3. Проверка закрытия окна SDL2/OpenGL для строки 106
    static bool WindowShouldClose() noexcept {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return true;
            }
        }
        return false;
    }

    // Возвращает путь к безопасной папке приложения для сохранений и конфигов
    static std::string GetSaveDirectoryPath();

    // Генерирует или считывает уникальный HWID устройства для защиты/идентификации
    static std::string GetDeviceHWID();
};

} // namespace Centralia
