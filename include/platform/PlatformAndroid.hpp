#pragma once
#include <string>
#include <cstdint>
#include <jni.h> // Нативные JNI-привязки для Android NDK

namespace Centralia {

class Platform {
private:
    static inline JavaVM* s_javaVM = nullptr;
    static inline jobject s_androidContext = nullptr;
    static inline std::string s_internalStoragePath = "/data/data/com.centralia.deadlend/files/";

public:
    Platform() = delete;
    ~Platform() = default;

    /**
     * @brief Инициализация контекста Android при старте NativeActivity / JNI_OnLoad
     */
    static void SetAndroidContext(JavaVM* vm, jobject context, const std::string& internalPath) noexcept {
        s_javaVM = vm;
        s_androidContext = context;
        s_internalStoragePath = internalPath;
        if (!s_internalStoragePath.empty() && s_internalStoragePath.back() != '/') {
            s_internalStoragePath += '/';
        };
    };

    /**
     * @brief Инициализация базовых систем платформы
     */
    [[nodiscard]] static bool Initialize() noexcept;

    /**
     * @brief Перенаправление логов движка напрямую в Android Logcat (Тег: CentraliaEngine)
     */
    static void Log(const std::string& message) noexcept;

    /**
     * @brief Генерация и чтение уникального HWID устройства на базе Android API
     */
    [[nodiscard]] static std::string GetDeviceHWID() noexcept;

    /**
     * @brief Возвращает путь к внутренней памяти Android для загрузки бинарных карт test.map
     */
    [[nodiscard]] static std::string GetSaveDirectoryPath() noexcept {
        return s_internalStoragePath;
    };

    /**
     * @brief Проверка флага закрытия окна приложения (для Android — сворачивание/уничтожение Activity)
     */
    [[nodiscard]] static bool WindowShouldClose() noexcept;
};

// Псевдонимы кодов клавиш для кроссплатформенной совместимости с Windows (InputController.cpp)
enum class KeyCode : uint32_t {
    Unknown = 0,
    W = 1, A = 2, S = 3, D = 4,
    Shift = 5, Ctrl = 6, Space = 7,
    G = 8, R = 9
};

}; // namespace Centralia
