#pragma once
#include "core/Math3D.hpp"
#include "platform/Platform.hpp" // Наш мастер-переключатель кодов клавиш
#include <SDL.h>
#include <string>
#include <vector>
#include <cstdint>
#include <cmath>

namespace Centralia {

// Перечисление поддерживаемых типов игровых манипуляторов
enum class ControllerType : uint8_t {
    KeyboardMouse,
    Gamepad_Xbox_PS,
    VR_AR_HandTracker
};

// Структура, агрегирующая все бинды действий из Fallout 76
struct GameplayActions {
    bool jump       = false; // Space / Кнопка A (Xbox)
    bool useAction  = false; // E / Кнопка X (Xbox) - обыск, открытие дверей
    bool reload     = false; // R / Кнопка X (Xbox) - перезарядка оружия
    bool useHeal    = false; // H / Крестовина (D-Pad) - быстрое лечение аптечкой
    bool ghostMode  = false; // Left Ctrl / Нажатие правого стика - скрытность
};

class InputController {
private:
    SDL_GameController* m_gamepad = nullptr;
    ControllerType m_currentType = ControllerType::KeyboardMouse;
    GameplayActions m_actions;

    // Внутренние переменные сглаживания мыши / VR-трекера
    float m_mouseSensitivity = 0.15f;
    float m_gamepadSensitivity = 3.0f;

public:
    inline InputController() noexcept {}
    
    inline ~InputController() {
        Shutdown();
    }

    // Запрет копирования контроллера ввода во избежание утечки системных дескрипторов
    InputController(const InputController&) = delete;
    InputController& operator=(const InputController&) = delete;

    // Первичный поиск контроллеров в Windows 10 / Linux
    class InputController {
    private:
        SDL_GameController* m_gamepad = nullptr;
        ControllerType m_currentType = ControllerType::KeyboardMouse;
        GameplayActions m_actions;
        float m_mouseSensitivity = 0.15f;
        float m_gamepadSensitivity = 3.0f;

    public:
        InputController() noexcept;
        ~InputController();
        InputController(const InputController&) = delete;
        InputController& operator=(const InputController&) = delete;

        void Initialize() noexcept;
        void ClearFrameTriggers() noexcept;
        void Update(SDL_Event& event) noexcept;
        [[nodiscard]] Vector3D GetMovementVector() const noexcept;
        void GetLookOffsets(float& outX, float& outY) const noexcept;
        [[nodiscard]] bool IsSprintPressed() const noexcept;
        void Shutdown() noexcept;

        [[nodiscard]] const GameplayActions& GetActions() const noexcept { return m_actions; }
        [[nodiscard]] ControllerType GetCurrentControllerType() const noexcept { return m_currentType; }
    };

    // Сброс триггеров одиночных действий — ИСПРАВЛЕНО: Вызывается ОДИН раз за кадр из Engine::Update
    inline void ClearFrameTriggers() noexcept {
        m_actions.jump = false;
        m_actions.useAction = false;
        m_actions.reload = false;
        m_actions.useHeal = false;
        m_actions.ghostMode = false;
    }
    
    // Опрос аппаратного состояния конкретного события SDL
    inline void Update(SDL_Event& event) noexcept {
        // Проверяем, не переключил ли пользователь устройство на лету
        if (event.type == SDL_CONTROLLERDEVICEADDED && !m_gamepad) {
            Initialize();
        }
        if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
            Shutdown();
            m_currentType = ControllerType::KeyboardMouse;
        }

        // 1. ПОЛУЧЕНИЕ СИГНАЛОВ ОТ ГЕЙМПАДА (Xbox / PS / Консольный режим)
        if (m_currentType == ControllerType::Gamepad_Xbox_PS && m_gamepad) {
            if (event.type == SDL_CONTROLLERBUTTONDOWN) {
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A) m_actions.jump = true;         
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_Y) m_actions.jump = true;         
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_X) m_actions.reload = true;       
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_B) m_actions.useAction = true;    
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSTICK) m_actions.ghostMode = true; 
                if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) m_actions.useHeal = true;   
            }
            return; 
        }

        // 2. РЕЗЕРВНЫЙ СЛОЙ OPENXR / VR РУК
        if (m_currentType == ControllerType::VR_AR_HandTracker) {
            return;
        }

        // 3. ПОЛУЧЕНИЕ СИГНАЛОВ ОТ КЛАВИАТУРЫ (Edge-triggered клики)
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.scancode) {
                case SDL_SCANCODE_SPACE:  m_actions.jump = true;       break; 
                case SDL_SCANCODE_E:      m_actions.useAction = true;  break; 
                case SDL_SCANCODE_R:      m_actions.reload = true;     break; 
                case SDL_SCANCODE_H:      m_actions.useHeal = true;    break; 
                case SDL_SCANCODE_LCTRL:  m_actions.ghostMode = true;  break; 
                default: break;
            }
        }
    }

    // Возвращает нормализованный вектор движения WASD / Стика / VR (-1.0f до 1.0f)
    inline Vector3D GetMovementVector() const noexcept {
        Vector3D direction(0.0f, 0.0f, 0.0f);

        if (m_currentType == ControllerType::Gamepad_Xbox_PS && m_gamepad) {
            int16_t rawX = SDL_GameControllerGetAxis(m_gamepad, SDL_CONTROLLER_AXIS_LEFTX);
            int16_t rawY = SDL_GameControllerGetAxis(m_gamepad, SDL_CONTROLLER_AXIS_LEFTY);

            // Мёртвая зона аналогового стика геймпада (защита от дрифта осей)
            if (std::abs(rawX) > 4200) direction.x = static_cast<float>(rawX) / 32767.0f;
            if (std::abs(rawY) > 4200) direction.z = static_cast<float>(-rawY) / 32767.0f; 
            return direction;
        }

        // Чтение клавиатуры WASD через прямое сканирование ОЗУ
        const uint8_t* state = SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_W]) direction.z += 1.0f; 
        if (state[SDL_SCANCODE_S]) direction.z -= 1.0f; 
        if (state[SDL_SCANCODE_A]) direction.x -= 1.0f; 
        if (state[SDL_SCANCODE_D]) direction.x += 1.0f; 

        return direction;
    }
    
    // Извлекает дельту смещения обзора (мышь / правый стик геймпада)
    inline void GetLookOffsets(float& outX, float& outY) const noexcept {
        outX = 0.0f;
        outY = 0.0f;

        if (m_currentType == ControllerType::Gamepad_Xbox_PS && m_gamepad) {
            int16_t axisX = SDL_GameControllerGetAxis(m_gamepad, SDL_CONTROLLER_AXIS_RIGHTX);
            int16_t axisY = SDL_GameControllerGetAxis(m_gamepad, SDL_CONTROLLER_AXIS_RIGHTY);

            if (std::abs(axisX) > 4000) outX = (static_cast<float>(axisX) / 32767.0f) * m_gamepadSensitivity;
            if (std::abs(axisY) > 4000) outY = (static_cast<float>(-axisY) / 32767.0f) * m_gamepadSensitivity;
        }
    }

    // Проверка зажатия модификатора бега (Left Shift / Нажатие левого стика)
    inline bool IsSprintPressed() const noexcept {
        if (m_currentType == ControllerType::Gamepad_Xbox_PS && m_gamepad) {
            return SDL_GameControllerGetButton(m_gamepad, SDL_CONTROLLER_BUTTON_LEFTSTICK) == 1;
        }
        
        const uint8_t* state = SDL_GetKeyboardState(NULL);
        return state[SDL_SCANCODE_LSHIFT] == 1;
    }

    inline void Shutdown() noexcept {
        if (m_gamepad) {
            SDL_GameControllerClose(m_gamepad);
            m_gamepad = nullptr;
        }
    }

    // Геттер для мгновенных триггерных действий (прыжки, обыск лута)
    [[nodiscard]] const GameplayActions& GetActions() const noexcept { return m_actions; }
    [[nodiscard]] ControllerType GetCurrentControllerType() const noexcept { return m_currentType; }
};

} // namespace Centralia
