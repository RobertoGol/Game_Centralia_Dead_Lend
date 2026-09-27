#pragma once
#include "platform/Platform.hpp"
#include <iostream>
#include <windows.h>
#include <shlobj.h>
#include <SDL.h>

namespace Centralia {

// Внутренний маппер сканкодов под Windows
inline static uint32_t MapEngineCodeToSDLWin32(KeyCode code) noexcept {
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

inline bool Platform::Initialize() noexcept {
    Log("Windows Subsystem: Крюки реестра Win32 и менеджер памяти MSVC переведены в номинальный режим.");
    return true;
}

inline void Platform::Log(const std::string& message) noexcept {
    std::string formatted = "[Centralia WIN32] " + message + "\n";
    std::cout << formatted;
    OutputDebugStringA(formatted.c_str()); // Вывод в консоль отладчика Visual Studio
}

inline bool Platform::IsKeyPressed(KeyCode code) noexcept {
    const uint8_t* state = SDL_GetKeyboardState(NULL);
    uint32_t sdlCode = MapEngineCodeToSDLWin32(code);
    return (sdlCode != SDL_SCANCODE_UNKNOWN) && (state[sdlCode] != 0);
}

inline bool Platform::IsKeyJustPressed(KeyCode code) noexcept {
    const uint8_t* state = SDL_GetKeyboardState(NULL);
    uint32_t sdlCode = MapEngineCodeToSDLWin32(code);
    return (sdlCode != SDL_SCANCODE_UNKNOWN) && (state[sdlCode] != 0);
}

inline bool Platform::WindowShouldClose() noexcept {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return true;
    }
    return false;
}

inline std::string Platform::GetSaveDirectoryPath() noexcept {
    char szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, szPath))) {
        std::string path = std::string(szPath) + "\\CentraliaDeadLend\\";
        CreateDirectoryA(path.c_str(), NULL);
        return path;
    }
    return ".\\saves\\";
}

inline std::string Platform::GetDeviceHWID() noexcept {
    HKEY hKey;
    char value[255];
    DWORD value_length = 255;
    // Считываем уникальный GUID установленной Windows 10/11 для проверки прав Модератора
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
