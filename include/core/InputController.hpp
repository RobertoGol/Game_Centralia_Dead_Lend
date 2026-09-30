#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "platform/Platform.hpp"

namespace Centralia {

enum class InputDeviceType : uint8_t {
    KeyboardMouse,
    Gamepad,
    SteeringWheel,
    Trackball,
    FingerTrackingSensor,     // Датчики движений пальцев / перчатки
    Sensor,                   // Общие сенсоры (гироскопы, акселерометры, тач-панели)
    PSPStyleController,       // Портативные контроллеры в стиле PSP
    GenericChineseController, // Китайские ноунейм-геймпады и кросс-платформенные клоны
    CustomTelemetryDevice     // Произвольные самоделки (COM-порт / сырой поток)
};

struct DeviceConnectionConfig {
    uint32_t deviceId;
    InputDeviceType type;
    std::string portOrPath;
    bool isConnected = false;
};

class InputController {
private:
    std::unordered_map<uint32_t, DeviceConnectionConfig> m_connectedDevices;
    std::unordered_map<std::string, float> m_axisStates;
    std::unordered_map<std::string, bool> m_buttonStates;

    InputController() noexcept = default;

public:
    ~InputController() = default;

    InputController(const InputController&) = delete;
    InputController& operator=(const InputController&) = delete;

    static InputController& GetInstance() noexcept {
        static InputController instance;
        return instance;
    }

    bool ConnectDevice(uint32_t deviceId, InputDeviceType type, const std::string& portOrPath) noexcept;
    bool DisconnectDevice(uint32_t deviceId) noexcept;
    void PollAllDevices() noexcept;

    [[nodiscard]] float GetAxis(const std::string& axisName) const noexcept;
    [[nodiscard]] bool IsButtonPressed(const std::string& buttonName) const noexcept;
    [[nodiscard]] bool IsDeviceConnected(uint32_t deviceId) noexcept;
    [[nodiscard]] size_t GetActiveDevicesCount() const noexcept { return m_connectedDevices.size(); }
};

} // namespace Centralia