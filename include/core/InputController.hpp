#pragma once
#include "core/Math3D.hpp"
#include <SDL2/SDL.h>

namespace Centralia {

// Перечисление поддерживаемых типов игровых манипуляторов
enum class ControllerType {
    KeyboardMouse,
    Gamepad_Xbox_PS,
    VR_AR_HandTracker
};

// Структура, агрегирующая все бинды действий из Fallout 76
struct GameplayActions {
    bool jump = false;       // Space / Кнопка A (Xbox)
    bool useAction = false;  // E / Кнопка X (Xbox) - обыск, открытие дверей
    bool reload = false;     // R / Кнопка X (Xbox) - перезарядка оружия
    bool useHeal = false;    // H / Крестовина (D-Pad) - быстрое лечение аптечкой
    bool ghostMode = false;  // Left Ctrl / Нажатие правого стика - скрытность
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
    InputController();
    ~InputController();

    // Первичный поиск контроллеров в Windows 10
    void Initialize();
    
    // Сброс и опрос аппаратного состояния на текущем кадре
    void Update(SDL_Event& event);

    // Возвращает нормализованный вектор движения WASD / Стика / VR (-1.0f до 1.0f)
    Vector3D GetMovementVector() const;
    
    // Извлекает дельту смещения обзора (мышь / правый стик геймпада)
    void GetLookOffsets(float& outX, float& outY) const;

    // Проверка зажатия модификатора бега (Left Shift / Нажатие левого стика)
    bool IsSprintPressed() const;

    // Геттер для мгновенных триггерных действий (прыжки, обыск лута)
    const GameplayActions& GetActions() const { return m_actions; }

    void Shutdown();
    ControllerType GetCurrentControllerType() const { return m_currentType; }
};

} // namespace Centralia