#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
    #include <windows.h>
    #include <shlobj.h>
    #include <SDL.h>
#elif defined(__ANDROID__)
    #include <android/log.h>
    #include <sys/system_properties.h>
    #include <jni.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <SDL.h>
#endif

namespace Centralia {

enum class KeyCode : uint32_t {
    Unknown = 0,
    W       = 1,
    A       = 2,
    S       = 3,
    D       = 4,
    Shift   = 5,
    Ctrl    = 6,
    Space   = 7,
    G       = 8,
    R       = 9
};

class Platform {
public:
    Platform() = delete;
    ~Platform() = delete;

    static bool Initialize() noexcept;
    static void Log(const std::string& message) noexcept;
    static bool IsKeyPressed(KeyCode code) noexcept;
    static bool IsKeyJustPressed(KeyCode code) noexcept;
    static bool WindowShouldClose() noexcept;
    static std::string GetSaveDirectoryPath() noexcept;
    static std::string GetDeviceHWID() noexcept;
};

} // namespace Centralia

#if defined(_WIN32)
    #include "platform/PlatformWin32.hpp"
#elif defined(__ANDROID__)
    #include "platform/PlatformAndroid.hpp"
#else
    #include "platform/PlatformLinux.hpp"
#endif
