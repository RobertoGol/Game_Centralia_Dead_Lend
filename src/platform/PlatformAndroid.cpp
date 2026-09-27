#include "platform/PlatformAndroid.hpp"
#include <android/log.h> // ПОДКЛЮЧЕНО: Дает доступ к функции __android_log_print
#include <sys/system_properties.h> // Дает доступ к чтению ro.product.model
#include <cstring>

#define LOG_TAG "CentraliaEngine"

namespace Centralia {

bool Platform::Initialize() noexcept {
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "[PLATFORM]: Android Native NDK Subsystem booted successfully.");
    return true;
}

void Platform::Log(const std::string& message) noexcept {
    // Пишем логи напрямую в системный буфер Android Logcat вместо std::cout
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s", message.c_str());
}

std::string Platform::GetDeviceHWID() noexcept {
    char propBuffer[PROP_VALUE_MAX];
    
    // 1. Пытаемся прочесть аппаратный серийный номер Android-устройства
    if (__system_property_get("ro.serialno", propBuffer) > 0 && std::strlen(propBuffer) > 0) {
        return std::string(propBuffer);
    }

    // 2. Фаллбек-вариант: если серийник скрыт политикой приватности, собираем HWID из модели процессора и платы
    std::string fallbackHwid = "ANDROID_";
    if (__system_property_get("ro.product.model", propBuffer) > 0) {
        fallbackHwid += propBuffer;
    } else {
        fallbackHwid += "UNKNOWN_BUILD_MODEL";
    }

    if (__system_property_get("ro.hardware", propBuffer) > 0) {
        fallbackHwid += "_" + std::string(propBuffer);
    }

    return fallbackHwid;
}

bool Platform::WindowShouldClose() noexcept {
    // На Android жизненным циклом рулит ОС, возвращаем false, пока Activity не уничтожено
    return false;
}

} // namespace Centralia
