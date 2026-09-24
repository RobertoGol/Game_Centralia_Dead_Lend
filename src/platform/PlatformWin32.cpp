#include "platform/Platform.hpp"
#include <iostream>
#include <windows.h>
#include <shlobj.h>

namespace Centralia {

bool Platform::Initialize() {
    Log("Windows 10 Platform initialized successfully.");
    return true;
}

void Platform::Log(const std::string& message) {
    std::string formatted = "[Centralia LOG] " + message + "\n";
    std::cout << formatted;
    OutputDebugStringA(formatted.c_str()); // Вывод в консоль отладчика Visual Studio/CLion
}

std::string Platform::GetSaveDirectoryPath() {
    char szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {
        std::string path = std::string(szPath) + "\\CentraliaDeadLend\\";
        CreateDirectoryA(path.c_str(), NULL);
        return path;
    }
    return ".\\saves\\";
}

std::string Platform::GetDeviceHWID() {
    HKEY hKey;
    char value[255];
    DWORD value_length = 255;
    // Считываем уникальный GUID установленной Windows 10
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0, KEY_READ | KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "MachineGuid", NULL, NULL, (LPBYTE)value, &value_length) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return std::string(value);
        }
        RegCloseKey(hKey);
    }
    return "WINDOWS_UNKNOWN_HWID";
}

} // namespace Centralia
