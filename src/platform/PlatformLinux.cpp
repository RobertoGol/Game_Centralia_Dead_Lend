#include "platform/Platform.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

namespace Centralia {

bool Platform::Initialize() {
    Log("Linux Platform initialized successfully.");
    return true;
}

void Platform::Log(const std::string& message) {
    std::cout << "[Centralia LOG] " << message << std::endl;
}

std::string Platform::GetSaveDirectoryPath() {
    const char* xdg_home = std::getenv("XDG_DATA_HOME");
    std::string path;
    if (xdg_home) {
        path = std::string(xdg_home) + "/CentraliaDeadLend/";
    } else {
        const char* home = std::getenv("HOME");
        path = home ? std::string(home) + "/.local/share/CentraliaDeadLend/" : "./saves/";
    }
    mkdir(path.c_str(), 0755);
    return path;
}

std::string Platform::GetDeviceHWID() {
    // Читаем machine-id, уникальный для каждой сборки Arch Linux
    std::ifstream idFile("/etc/machine-id");
    std::string hwid;
    if (idFile >> hwid) {
        return hwid;
    }
    return "LINUX_UNKNOWN_HWID";
}

} // namespace Centralia
