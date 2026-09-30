#include "InputController.hpp"
#include "platform/Platform.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS & MATH UTILITIES
// ============================================================================

namespace {
    constexpr int MAX_KEYS = 512;
    constexpr int MAX_MOUSE_BUTTONS = 8;
    constexpr int MAX_GAMEPADS = 4;
    constexpr int MAX_GAMEPAD_BUTTONS = 24;
    constexpr int MAX_GAMEPAD_AXES = 6;
    
    constexpr float DEFAULT_DEADZONE = 0.15f;
    constexpr float DEFAULT_TRIGGER_THRESHOLD = 0.1f;

    // Вспомогательная функция для радиальной мертвой зоны стиков геймпада
    void ApplyRadialDeadzone(float& outX, float& outY, float deadzone) {
        float magnitude = std::sqrt(outX * outX + outY * outY);
        if (magnitude < deadzone) {
            outX = 0.0f;
            outY = 0.0f;
        } else {
            // Нормализация выхода за пределами мертвой зоны (плавный старт от 0 до 1)
            float normalizedMagnitude = (magnitude - deadzone) / (1.0f - deadzone);
            outX = (outX / magnitude) * normalizedMagnitude;
            outY = (outY / magnitude) * normalizedMagnitude;
        }
    }
}

// ============================================================================
// SECTION 2: CONSTRUCTORS, DESTRUCTORS & MEMORY ALLOCATION
// ============================================================================

InputController::InputController() 
    : m_mouseCaptured(false),
      m_mouseX(0.0f), m_mouseY(0.0f),
      m_mouseDeltaX(0.0f), m_mouseDeltaY(0.0f),
      m_mouseScrollX(0.0f), m_mouseScrollY(0.0f),
      m_mouseSensitivity(1.0f),
      m_invertMouseY(false),
      m_rawInputEnabled(true)
{
    // Инициализация буферов клавиатуры
    m_currentKeyStates = new bool[MAX_KEYS];
    m_previousKeyStates = new bool[MAX_KEYS];
    std::memset(m_currentKeyStates, 0, MAX_KEYS * sizeof(bool));
    std::memset(m_previousKeyStates, 0, MAX_KEYS * sizeof(bool));

    // Инициализация буферов мыши
    m_currentMouseStates = new bool[MAX_MOUSE_BUTTONS];
    m_previousMouseStates = new bool[MAX_MOUSE_BUTTONS];
    std::memset(m_currentMouseStates, 0, MAX_MOUSE_BUTTONS * sizeof(bool));
    std::memset(m_previousMouseStates, 0, MAX_MOUSE_BUTTONS * sizeof(bool));

    // Инициализация структуры геймпадов
    m_gamepads = new GamepadState[MAX_GAMEPADS];
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        m_gamepads[i].isConnected = false;
        m_gamepads[i].leftMotorVibration = 0.0f;
        m_gamepads[i].rightMotorVibration = 0.0f;
        m_gamepads[i].vibrationDurationTimer = 0.0f;
        std::memset(m_gamepads[i].currentButtons, 0, MAX_GAMEPAD_BUTTONS * sizeof(bool));
        std::memset(m_gamepads[i].previousButtons, 0, MAX_GAMEPAD_BUTTONS * sizeof(bool));
        std::memset(m_gamepads[i].axes, 0, MAX_GAMEPAD_AXES * sizeof(float));
    }

    m_actionBindings.clear();
    SetupDefaultActionBindings();

    Platform::Log("[INPUT CONTROLLER]: Подсистема ввода успешно распределила буферы в памяти.");
}

InputController::~InputController() {
    delete[] m_currentKeyStates;
    delete[] m_previousKeyStates;
    delete[] m_currentMouseStates;
    delete[] m_previousMouseStates;
    delete[] m_gamepads;
    
    m_actionBindings.clear();
    Platform::Log("[INPUT CONTROLLER]: Буферы ввода уничтожены, память освобождена.");
}

void InputController::InitializeDevices() {
    Platform::Log("[INPUT CONTROLLER]: Поиск и регистрация устройств ввода (HID)...");
    
    // В реальном движке здесь происходит опрос XInput/DirectInput/udev
    // Эмулируем нахождение одного геймпада
    m_gamepads[0].isConnected = true;
    Platform::Log("[INPUT CONTROLLER]: Обнаружен XInput совместимый контроллер (Порт 0).");
    
    if (m_rawInputEnabled) {
        Platform::EnableRawMouseInput();
        Platform::Log("[INPUT CONTROLLER]: Режим Raw Input активирован (акселерация мыши ОС отключена).");
    }
}

// ============================================================================
// SECTION 3: CORE EVENT POLLING & BUFFER SWAPPING
// ============================================================================

void InputController::PollEvents() {
    // 1. Копирование текущих состояний в предыдущие (для детектирования Pressed/Released)
    std::memcpy(m_previousKeyStates, m_currentKeyStates, MAX_KEYS * sizeof(bool));
    std::memcpy(m_previousMouseStates, m_currentMouseStates, MAX_MOUSE_BUTTONS * sizeof(bool));
    
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        if (m_gamepads[i].isConnected) {
            std::memcpy(m_gamepads[i].previousButtons, m_gamepads[i].currentButtons, MAX_GAMEPAD_BUTTONS * sizeof(bool));
        }
    }

    // 2. Сброс дельт мыши и скролла (они актуальны только один кадр)
    m_mouseDeltaX = 0.0f;
    m_mouseDeltaY = 0.0f;
    m_mouseScrollX = 0.0f;
    m_mouseScrollY = 0.0f;

    // 3. Обновление таймеров виброотдачи геймпадов
    UpdateHapticFeedback(0.016f); // Предполагаем дельту кадра ~16ms для таймера вибро
}

// ============================================================================
// SECTION 4: INJECTION APIS (CALLED BY PLATFORM WINDOW MESSAGE LOOP)
// ============================================================================

void InputController::InjectKeyDown(uint16_t keycode) noexcept {
    if (keycode < MAX_KEYS) {
        m_currentKeyStates[keycode] = true;
    }
}

void InputController::InjectKeyUp(uint16_t keycode) noexcept {
    if (keycode < MAX_KEYS) {
        m_currentKeyStates[keycode] = false;
    }
}

void InputController::InjectMouseMove(float x, float y, float deltaX, float deltaY) noexcept {
    m_mouseX = x;
    m_mouseY = y;
    
    // Применяем чувствительность и опциональную инверсию оси Y
    m_mouseDeltaX = deltaX * m_mouseSensitivity;
    m_mouseDeltaY = deltaY * m_mouseSensitivity * (m_invertMouseY ? -1.0f : 1.0f);
}

void InputController::InjectMouseButton(uint8_t buttonId, bool isDown) noexcept {
    if (buttonId < MAX_MOUSE_BUTTONS) {
        m_currentMouseStates[buttonId] = isDown;
    }
}

void InputController::InjectMouseScroll(float scrollX, float scrollY) noexcept {
    m_mouseScrollX = scrollX;
    m_mouseScrollY = scrollY;
}

void InputController::InjectGamepadButton(uint8_t gamepadId, uint8_t buttonId, bool isDown) noexcept {
    if (gamepadId < MAX_GAMEPADS && buttonId < MAX_GAMEPAD_BUTTONS) {
        if (m_gamepads[gamepadId].isConnected) {
            m_gamepads[gamepadId].currentButtons[buttonId] = isDown;
        }
    }
}

void InputController::InjectGamepadAxis(uint8_t gamepadId, uint8_t axisId, float value) noexcept {
    if (gamepadId < MAX_GAMEPADS && axisId < MAX_GAMEPAD_AXES) {
        if (m_gamepads[gamepadId].isConnected) {
            m_gamepads[gamepadId].axes[axisId] = std::clamp(value, -1.0f, 1.0f);
        }
    }
}

void InputController::InjectGamepadConnection(uint8_t gamepadId, bool connected) noexcept {
    if (gamepadId < MAX_GAMEPADS) {
        m_gamepads[gamepadId].isConnected = connected;
        if (connected) {
            Platform::Log("[INPUT SYSTEM]: Геймпад #" + std::to_string(gamepadId) + " подключен.");
        } else {
            // Сброс состояния при отключении
            std::memset(m_gamepads[gamepadId].currentButtons, 0, MAX_GAMEPAD_BUTTONS * sizeof(bool));
            std::memset(m_gamepads[gamepadId].axes, 0, MAX_GAMEPAD_AXES * sizeof(float));
            Platform::Log("[INPUT SYSTEM]: Геймпад #" + std::to_string(gamepadId) + " отключен.");
        }
    }
}

// ============================================================================
// SECTION 5: KEYBOARD & MOUSE QUERY APIS
// ============================================================================

bool InputController::IsKeyDown(KeyCode key) const noexcept {
    uint16_t code = static_cast<uint16_t>(key);
    return (code < MAX_KEYS) ? m_currentKeyStates[code] : false;
}

bool InputController::IsKeyPressed(KeyCode key) const noexcept {
    uint16_t code = static_cast<uint16_t>(key);
    if (code >= MAX_KEYS) return false;
    return m_currentKeyStates[code] && !m_previousKeyStates[code];
}

bool InputController::IsKeyReleased(KeyCode key) const noexcept {
    uint16_t code = static_cast<uint16_t>(key);
    if (code >= MAX_KEYS) return false;
    return !m_currentKeyStates[code] && m_previousKeyStates[code];
}

bool InputController::IsMouseButtonDown(MouseButton button) const noexcept {
    uint8_t code = static_cast<uint8_t>(button);
    return (code < MAX_MOUSE_BUTTONS) ? m_currentMouseStates[code] : false;
}

bool InputController::IsMouseButtonPressed(MouseButton button) const noexcept {
    uint8_t code = static_cast<uint8_t>(button);
    if (code >= MAX_MOUSE_BUTTONS) return false;
    return m_currentMouseStates[code] && !m_previousMouseStates[code];
}

bool InputController::IsMouseButtonReleased(MouseButton button) const noexcept {
    uint8_t code = static_cast<uint8_t>(button);
    if (code >= MAX_MOUSE_BUTTONS) return false;
    return !m_currentMouseStates[code] && m_previousMouseStates[code];
}

void InputController::GetMousePosition(float& outX, float& outY) const noexcept {
    outX = m_mouseX;
    outY = m_mouseY;
}

void InputController::GetMouseDelta(float& outDeltaX, float& outDeltaY) const noexcept {
    outDeltaX = m_mouseDeltaX;
    outDeltaY = m_mouseDeltaY;
}

void InputController::SetMouseCapture(bool capture) noexcept {
    m_mouseCaptured = capture;
    Platform::SetCursorVisible(!capture);
    Platform::LockCursorToWindow(capture);
    Platform::Log(capture ? "[INPUT]: Курсор мыши захвачен движком." : "[INPUT]: Курсор мыши освобожден.");
}

// ============================================================================
// SECTION 6: GAMEPAD & HAPTIC FEEDBACK (RUMBLE) APIS
// ============================================================================

bool InputController::IsGamepadConnected(uint8_t gamepadId) const noexcept {
    return (gamepadId < MAX_GAMEPADS) ? m_gamepads[gamepadId].isConnected : false;
}

bool InputController::IsGamepadButtonDown(uint8_t gamepadId, GamepadButton button) const noexcept {
    if (gamepadId >= MAX_GAMEPADS || !m_gamepads[gamepadId].isConnected) return false;
    uint8_t code = static_cast<uint8_t>(button);
    return (code < MAX_GAMEPAD_BUTTONS) ? m_gamepads[gamepadId].currentButtons[code] : false;
}

bool InputController::IsGamepadButtonPressed(uint8_t gamepadId, GamepadButton button) const noexcept {
    if (gamepadId >= MAX_GAMEPADS || !m_gamepads[gamepadId].isConnected) return false;
    uint8_t code = static_cast<uint8_t>(button);
    if (code >= MAX_GAMEPAD_BUTTONS) return false;
    return m_gamepads[gamepadId].currentButtons[code] && !m_gamepads[gamepadId].previousButtons[code];
}

void InputController::GetGamepadLeftStick(uint8_t gamepadId, float& outX, float& outY) const noexcept {
    outX = 0.0f; outY = 0.0f;
    if (gamepadId >= MAX_GAMEPADS || !m_gamepads[gamepadId].isConnected) return;
    
    outX = m_gamepads[gamepadId].axes[0]; // Лев. Стик X
    outY = m_gamepads[gamepadId].axes[1]; // Лев. Стик Y
    ApplyRadialDeadzone(outX, outY, DEFAULT_DEADZONE);
}

void InputController::GetGamepadRightStick(uint8_t gamepadId, float& outX, float& outY) const noexcept {
    outX = 0.0f; outY = 0.0f;
    if (gamepadId >= MAX_GAMEPADS || !m_gamepads[gamepadId].isConnected) return;
    
    outX = m_gamepads[gamepadId].axes[2]; // Прав. Стик X
    outY = m_gamepads[gamepadId].axes[3]; // Прав. Стик Y
    ApplyRadialDeadzone(outX, outY, DEFAULT_DEADZONE);
}

float InputController::GetGamepadLeftTrigger(uint8_t gamepadId) const noexcept {
    if (gamepadId >= MAX_GAMEPADS || !m_gamepads[gamepadId].isConnected) return 0.0f;
    float val = m_gamepads[gamepadId].axes[4];
    return (val > DEFAULT_TRIGGER_THRESHOLD) ? val : 0.0f;
}

float InputController::GetGamepadRightTrigger(uint8_t gamepadId) const noexcept {
    if (gamepadId >= MAX_GAMEPADS || !m_gamepads[gamepadId].isConnected) return 0.0f;
    float val = m_gamepads[gamepadId].axes[5];
    return (val > DEFAULT_TRIGGER_THRESHOLD) ? val : 0.0f;
}

void InputController::SetGamepadRumble(uint8_t gamepadId, float leftMotor, float rightMotor, float durationSeconds) noexcept {
    if (gamepadId >= MAX_GAMEPADS || !m_gamepads[gamepadId].isConnected) return;
    
    GamepadState& pad = m_gamepads[gamepadId];
    pad.leftMotorVibration = std::clamp(leftMotor, 0.0f, 1.0f);
    pad.rightMotorVibration = std::clamp(rightMotor, 0.0f, 1.0f);
    pad.vibrationDurationTimer = durationSeconds;

    // Системный вызов API (например, XInputSetState в Windows)
    Platform::SetGamepadVibration(gamepadId, pad.leftMotorVibration, pad.rightMotorVibration);
}

void InputController::UpdateHapticFeedback(float deltaTime) noexcept {
    for (int i = 0; i < MAX_GAMEPADS; ++i) {
        GamepadState& pad = m_gamepads[i];
        if (pad.isConnected && pad.vibrationDurationTimer > 0.0f) {
            pad.vibrationDurationTimer -= deltaTime;
            if (pad.vibrationDurationTimer <= 0.0f) {
                // Плавное затухание моторов при окончании таймера
                pad.vibrationDurationTimer = 0.0f;
                pad.leftMotorVibration = 0.0f;
                pad.rightMotorVibration = 0.0f;
                Platform::SetGamepadVibration(i, 0.0f, 0.0f);
            }
        }
    }
}

// ============================================================================
// SECTION 7: ACTION MAPPING & BINDING SYSTEM
// ============================================================================

void InputController::SetupDefaultActionBindings() {
    m_actionBindings.clear();

    // Экшены движения (Movement)
    BindAction("MoveForward", KeyCode::W, KeyCode::Up, GamepadButton::DPadUp);
    BindAction("MoveBackward", KeyCode::S, KeyCode::Down, GamepadButton::DPadDown);
    BindAction("MoveLeft", KeyCode::A, KeyCode::Left, GamepadButton::DPadLeft);
    BindAction("MoveRight", KeyCode::D, KeyCode::Right, GamepadButton::DPadRight);
    
    // Экшены выживания (Gameplay)
    BindAction("Jump", KeyCode::Space, KeyCode::None, GamepadButton::A);
    BindAction("Crouch", KeyCode::Ctrl, KeyCode::C, GamepadButton::B);
    BindAction("Sprint", KeyCode::Shift, KeyCode::None, GamepadButton::LeftThumb);
    BindAction("Interact", KeyCode::E, KeyCode::Enter, GamepadButton::X);
    
    // Боевые экшены (Combat)
    BindAction("Reload", KeyCode::R, KeyCode::None, GamepadButton::Y);
    BindAction("Melee", KeyCode::V, KeyCode::None, GamepadButton::RightThumb);
    BindAction("OpenInventory", KeyCode::Tab, KeyCode::I, GamepadButton::Start);
    BindAction("Pause", KeyCode::Escape, KeyCode::None, GamepadButton::Back);

    Platform::Log("[ACTION MAPPING]: Базовые привязки управления (Default Binds) успешно сгенерированы.");
}

void InputController::BindAction(const std::string& actionName, KeyCode primaryKey, KeyCode secondaryKey, GamepadButton padButton) {
    InputAction action;
    action.name = actionName;
    action.primaryKey = primaryKey;
    action.secondaryKey = secondaryKey;
    action.gamepadButton = padButton;
    
    m_actionBindings[actionName] = action;
}

void InputController::UnbindAction(const std::string& actionName) {
    auto it = m_actionBindings.find(actionName);
    if (it != m_actionBindings.end()) {
        m_actionBindings.erase(it);
        Platform::Log("[ACTION MAPPING]: Экшен '" + actionName + "' удален из конфигурации.");
    }
}

bool InputController::IsActionDown(const std::string& actionName) const {
    auto it = m_actionBindings.find(actionName);
    if (it == m_actionBindings.end()) return false;

    const InputAction& action = it->second;
    
    if (IsKeyDown(action.primaryKey) || IsKeyDown(action.secondaryKey)) return true;
    if (IsGamepadButtonDown(0, action.gamepadButton)) return true; // Проверка 1-го геймпада

    return false;
}

bool InputController::IsActionPressed(const std::string& actionName) const {
    auto it = m_actionBindings.find(actionName);
    if (it == m_actionBindings.end()) return false;

    const InputAction& action = it->second;
    
    if (IsKeyPressed(action.primaryKey) || IsKeyPressed(action.secondaryKey)) return true;
    if (IsGamepadButtonPressed(0, action.gamepadButton)) return true;

    return false;
}

bool InputController::IsActionReleased(const std::string& actionName) const {
    auto it = m_actionBindings.find(actionName);
    if (it == m_actionBindings.end()) return false;

    const InputAction& action = it->second;
    
    if (IsKeyReleased(action.primaryKey) || IsKeyReleased(action.secondaryKey)) return true;
    // Для релиза на геймпаде нужна дополнительная логика проверки (упрощенно)
    
    return false;
}

// ============================================================================
// SECTION 8: CONFIGURATION SERIALIZATION (LOADING/SAVING BINDS)
// ============================================================================

bool InputController::SaveBindingsToFile(const std::string& filepath) const {
    std::ofstream outFile(filepath);
    if (!outFile.is_open()) {
        Platform::Log("[INPUT SAVE ERROR]: Невозможно открыть файл для записи профиля управления: " + filepath);
        return false;
    }

    outFile << "# CENTRALIA - INPUT BINDINGS PROFILE\n";
    outFile << "# Format: ActionName = PrimaryKeyID, SecondaryKeyID, GamepadButtonID\n\n";

    for (const auto& [name, action] : m_actionBindings) {
        outFile << name << "=" 
                << static_cast<int>(action.primaryKey) << "," 
                << static_cast<int>(action.secondaryKey) << "," 
                << static_cast<int>(action.gamepadButton) << "\n";
    }

    outFile.close();
    Platform::Log("[INPUT SYSTEM]: Кастомные привязки управления успешно сохранены в " + filepath);
    return true;
}

bool InputController::LoadBindingsFromFile(const std::string& filepath) {
    std::ifstream inFile(filepath);
    if (!inFile.is_open()) {
        Platform::Log("[INPUT LOAD WARNING]: Файл профиля управления не найден. Используются дефолтные привязки.");
        return false;
    }

    std::string line;
    while (std::getline(inFile, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t equalPos = line.find('=');
        if (equalPos == std::string::npos) continue;

        std::string actionName = line.substr(0, equalPos);
        std::string valuesStr = line.substr(equalPos + 1);

        // Парсинг 3х CSV значений (PrimaryKey, SecondaryKey, GamepadButton)
        std::stringstream ss(valuesStr);
        std::string token;
        int parsedValues[3] = {0, 0, 0};
        int i = 0;
        
        while (std::getline(ss, token, ',') && i < 3) {
            parsedValues[i++] = std::stoi(token);
        }

        BindAction(actionName, 
                   static_cast<KeyCode>(parsedValues[0]), 
                   static_cast<KeyCode>(parsedValues[1]), 
                   static_cast<GamepadButton>(parsedValues[2]));
    }

    inFile.close();
    Platform::Log("[INPUT SYSTEM]: Пользовательские привязки успешно загружены из " + filepath);
    return true;
}

} // namespace Centralia