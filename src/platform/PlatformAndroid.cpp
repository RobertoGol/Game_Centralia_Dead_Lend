#include "platform/Platform.hpp"
#include <android/log.h>

#define LOG_TAG "CentraliaEngine"

namespace Centralia {

bool Platform::Initialize() {
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Android Platform initialized successfully.");
    return true;
}

void Platform::Log(const std::string& message) {
    __android_log_print(ANDROID_LOG_INFO, LOG_TAG, "%s", message.c_str());
}

std::string Platform::GetSaveDirectoryPath() {
    // В Android путь передается из Java-слоя (через context.getFilesDir().getAbsolutePath())
    // На этапе чистого NDK без JNI используем стандартную внутреннюю временную директорию
    return "/data/data/com.robertogol.centralia/files/";
}

std::string Platform::GetDeviceHWID() {
    // В Android 10-13 доступ к железу ограничен. Идентификатор генерируется на Java через Settings.Secure.ANDROID_ID 
    // и пробрасывается в C++. Возвращаем плейсхолдер для нативного слоя.
    return "ANDROID_NATIVE_HWID_LAYER";
}

} // namespace Centralia
