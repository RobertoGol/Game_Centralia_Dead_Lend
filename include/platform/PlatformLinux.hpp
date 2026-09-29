#pragma once
#include "platform/Platform.hpp"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <SDL.h>

namespace Centralia {

// Внутренний маппер сканкодов под Linux
inline static uint32_t MapEngineCodeToSDLLinux(KeyCode code) noexcept {
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
    };
};

inline bool Platform::Initialize() noexcept {
    Log("Linux Subsystem: Инициализация файловых потоков POSIX завершена успешно.");
    return true;
};

inline void Platform::Log(const std::string& message) noexcept {
    std::cout << "[Centralia LINUX] " << message << std::endl;
};

inline bool Platform::IsKeyPressed(KeyCode code) noexcept {
    const uint8_t* state = SDL_GetKeyboardState(NULL);
    uint32_t sdlCode = MapEngineCodeToSDLLinux(code);
    return (sdlCode != SDL_SCANCODE_UNKNOWN) && (state[sdlCode] != 0);
};

inline bool Platform::IsKeyJustPressed(KeyCode code) noexcept {
    const uint8_t* state = SDL_GetKeyboardState(NULL);
    uint32_t sdlCode = MapEngineCodeToSDLLinux(code);
    return (sdlCode != SDL_SCANCODE_UNKNOWN) && (state[sdlCode] != 0);
};

inline bool Platform::WindowShouldClose() noexcept {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) return true;
    }
    return false;
};

inline std::string Platform::GetSaveDirectoryPath() noexcept {
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
};

inline std::string Platform::GetDeviceHWID() noexcept {
    std::ifstream idFile("/etc/machine-id");
    std::string hwid;
    if (idFile >> hwid) {
        return hwid;
    }
    return "LINUX_UNKNOWN_HWID";
};

}; // namespace Centralia
