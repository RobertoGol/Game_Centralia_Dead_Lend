#include "core/InputController.hpp"
#include "platform/Platform.hpp"
#include <cmath>

namespace Centralia {

InputController::InputController() {}
InputController::~InputController() { Shutdown(); }

void InputController::Initialize() {
    // Сканируем системные порты Windows на наличие подключенных геймпадов
    if (SDL_NumJoysticks() > 0) {
        for (int i = 0; i < SDL_NumJoysticks(); ++i) {
            if (SDL_IsGameController(i)) {
                m_gamepad = SDL_GameControllerOpen(i);
                if (m_gamepad) {
                    m_currentType = ControllerType::Gamepad_Xbox_PS;
                    Platform::Log("InputController: Hardware Gamepad detected [" + 
                                  std::string(SDL_GameControllerName(m_gamepad)) + "]. Fallout 76 map active.");
                    break;
                }
            }
        }
    }

    if (!m_gamepad) {
        m_currentType = ControllerType::KeyboardMouse;
        Platform::Log("InputController: No gamepads found. Mapping to Keyboard & Mouse (Fallout 76 bindings).");
    }
}

void InputController::Update(SDL_Event& event) {
    // Очищаем триггеры мгновенных одиночных действий перед опросом нового кадра
    m_actions.jump = false;
    m_actions.useAction = false;
    m_actions.reload = false;
    m_actions.useHeal = false;
    m_actions.ghostMode = false;

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
            if (event.cbutton.button == SDL_CONTROLLER_BUTTON_A) m_actions.jump = true;         // Прыжок
            if (event.cbutton.button == SDL_CONTROLLER_BUTTON_Y) m_actions.jump = true;         // Альтернатива прыжка
            if (event.cbutton.button == SDL_CONTROLLER_BUTTON_X) m_actions.reload = true;       // Перезарядка (R)
            if (event.cbutton.button == SDL_CONTROLLER_BUTTON_B) m_actions.useAction = true;    // Взаимодействие (E)
            if (event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSTICK) m_actions.ghostMode = true; // Скрытность (Ctrl)
            if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) m_actions.useHeal = true;   // Лечение (H)
        }
        return; // Если активен геймпад, игнорируем клавиатуру
    }

    // 2. ПОЛУЧЕНИЕ СИГНАЛОВ ОТ VR / AR ПЕРЧАТОК (Резервный слой OpenXR)
    if (m_currentType == ControllerType::VR_AR_HandTracker) {
        // Здесь будет считываться пространственная дельта координат рук трекеров.
        // Оставляем пустым для будущей интеграции AR-модулей.
        return;
    }

    // 3. ПОЛУЧЕНИЕ СИГНАЛОВ ОТ КЛАВИАТУРЫ (Fallout 76 ПК-раскладка)
    if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.scancode) {
            case SDL_SCANCODE_SPACE:  m_actions.jump = true;       break; // Прыжок
            case SDL_SCANCODE_E:      m_actions.useAction = true;  break; // Использовать / Обыскать ящик
            case SDL_SCANCODE_R:      m_actions.reload = true;     break; // Перезарядка
            case SDL_SCANCODE_H:      m_actions.useHeal = true;    break; // Быстрое лечение аптечкой
            case SDL_SCANCODE_LCTRL:  m_actions.ghostMode = true;  break; // Скрытность (Ghost Mode)
            default: break;
        }
    }
}

Vector3D InputController::GetMovementVector() const {
    Vector3D direction(0.0f, 0.0f, 0.0f);

    // Чтение осей геймпада
    if (m_currentType == ControllerType::Gamepad_Xbox_PS && m_gamepad) {
        int16_t rawX = SDL_GameControllerGetAxis(m_gamepad, SDL_CONTROLLER_AXIS_LEFTX);
        int16_t rawY = SDL_GameControllerGetAxis(m_gamepad, SDL_CONTROLLER_AXIS_LEFTY);

        // Мёртвая зона аналогового стика геймпада (защита от дрифта осей)
        if (std::abs(rawX) > 4200) direction.x = static_cast<float>(rawX) / 32767.0f;
        if (std::abs(rawY) > 4200) direction.z = static_cast<float>(-rawY) / 32767.0f; // Переворачиваем Y ось для 3D
        return direction;
    }

    // Чтение клавиатуры WASD
    const Uint8* state = SDL_GetKeyboardState(NULL);
    if (state[SDL_SCANCODE_W]) direction.z += 1.0f; // Вперед
    if (state[SDL_SCANCODE_S]) direction.z -= 1.0f; // Назад
    if (state[SDL_SCANCODE_A]) direction.x -= 1.0f; // Влево (стрейф)
    if (state[SDL_SCANCODE_D]) direction.x += 1.0f; // Вправо (стрейф)

    return direction;
}

void InputController::GetLookOffsets(float& outX, float& outY) const {
    outX = 0.0f;
    outY = 0.0f;

    // Чтение правого стика обзора геймпада
    if (m_currentType == ControllerType::Gamepad_Xbox_PS && m_gamepad) {
        int16_t axisX = SDL_GameControllerGetAxis(m_gamepad, SDL_CONTROLLER_AXIS_RIGHTX);
        int16_t axisY = SDL_GameControllerGetAxis(m_gamepad, SDL_CONTROLLER_AXIS_RIGHTY);

        if (std::abs(axisX) > 4000) outX = (static_cast<float>(axisX) / 32767.0f) * m_gamepadSensitivity;
        if (std::abs(axisY) > 4000) outY = (static_cast<float>(-axisY) / 32767.0f) * m_gamepadSensitivity;
        return;
    }

    // Если геймпад не подключен, то смещение мыши считывается через SDL_MOUSEMOTION 
    // напрямую в цикле обработки событий main.cpp с множителем m_mouseSensitivity.
}

bool InputController::IsSprintPressed() const {
    if (m_currentType == ControllerType::Gamepad_Xbox_PS && m_gamepad) {
        // Спринт в Fallout 76 на геймпаде — это нажатие на Левый Стик (L3)
        return SDL_GameControllerGetButton(m_gamepad, SDL_CONTROLLER_BUTTON_LEFTSTICK) == 1;
    }
    
    // Спринт на ПК — удержание Левого Shift
    const Uint8* state = SDL_GetKeyboardState(NULL);
    return state[SDL_SCANCODE_LSHIFT] == 1;
}

void InputController::Shutdown() {
    if (m_gamepad) {
        SDL_GameControllerClose(m_gamepad);
        m_gamepad = nullptr;
    }
}

} // namespace Centralia