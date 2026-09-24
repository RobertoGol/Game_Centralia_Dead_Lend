#pragma once
#include <string>

namespace Centralia {

class Platform {
public:
    // Инициализация низкоуровневых систем (файловый менеджер, права доступа)
    static bool Initialize();

    // Кроссплатформенный вывод в системный лог (Терминал / Debug Output / Logcat)
    static void Log(const std::string& message);

    // Возвращает путь к безопасной папке приложения для сохранений и конфигов
    static std::string GetSaveDirectoryPath();

    // Генерирует или считывает уникальный HWID устройства для защиты/идентификации
    static std::string GetDeviceHWID();
};

} // namespace Centralia
