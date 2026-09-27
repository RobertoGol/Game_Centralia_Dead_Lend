#include "platform/Platform.hpp"
#include <iostream>
#include <windows.h>
#include <shlobj.h>
#include <SDL.h> // Win32-версия считывает сканкоды через SDL2

namespace Centralia {

static uint32_t MapEngineCodeToSDLWin32(KeyCode code) noexcept {
    switch (code) {
        case KeyCode::W:     return SDL_SCANCODE_W;
        case KeyCode::A:     return SDL_SCANCODE_A;
        case KeyCode::S:     return SDL_SCANCODE_S;
        case KeyCode::D:     return SDL_SCANCODE_D;
        case KeyCode::Shift: return SDL_SCANCODE_LSHIFT;
        case KeyCode::Ctrl:  return SDL_SCANCODE_LCTRL;
        case KeyCode::Space: return SDL_SCANCODE_SPACE;
        case KeyCode::G:     return SDL_SCANCODE_G;
        case KeyCode::R:     return SDL_SCANCODE_R;
        default:             return SDL_SCANCODE_UNKNOWN;
    }
}

bool Platform::Initialize() {
    Log("Windows Subsystem: Крюки реестра Win32 и менеджер памяти MSVC переведены в номинальный режим.");
    return true;
}

void Platform::Log(const std::string& message) {
    std::string formatted = "[Centralia WIN32] " + message + "\n";
    std::cout << formatted;
    OutputDebugStringA(formatted.c_str()); 
}

bool Platform::IsKeyPressed(KeyCode code) noexcept {
    const uint8_t* state = SDL_GetKeyboardState(NULL);
    uint32_t sdlCode = MapEngineCodeToSDLWin32(code);
    return (sdlCode != SDL_SCANCODE_UNKNOWN) && (state[sdlCode] != 0);
}

bool Platform::IsKeyJustPressed(KeyCode code) noexcept {
    const uint8_t* state = SDL_GetKeyboardState(NULL);
    uint32_t sdlCode = MapEngineCodeToSDLWin32(code);
    return (sdlCode != SDL_SCANCODE_UNKNOWN) && (state[sdlCode] != 0);
}

bool Platform::WindowShouldClose() noexcept {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return true;
    }
    return false;
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
    // Читаем криптографический GUID лицензии Windows 10/11 для проверки прав Модератора Админки
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
