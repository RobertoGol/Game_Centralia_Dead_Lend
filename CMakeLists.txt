cmake_minimum_required(VERSION 3.16)
project(Game_Centralia_Dead_Lend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#include(FetchContent)

# Автоматически скачиваем и собираем GLAD для поддержки Modern OpenGL (VAO/VBO/Шейдеры)
#FetchContent_Declare(
#    glad
#    URL https://github.com/Dav1dde/glad/archive/refs/tags/v0.1.36.zip
#)
#FetchContent_MakeAvailable(glad)

# Папки для поиска заголовочных файлов
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${glad_SOURCE_DIR}/include)

# Полный список исходных файлов ядра игры (соответствует твоему дереву папок)
set(ENGINE_SOURCES
    src/glad.c
    src/core/Engine.cpp
    src/core/MemoryManager.cpp
    src/core/NetworkProtocol.cpp
    src/core/NetworkSocket.cpp
    src/core/ConfigSystem.cpp
    src/core/LoginSystem.cpp
    src/core/InputController.cpp
    src/gameplay/Player.cpp
    src/gameplay/ItemDatabase.cpp
    src/gameplay/CraftingManager.cpp
    src/gameplay/ProceduralMotion.cpp 
    src/gameplay/ServerShop.cpp
    src/gameplay/WeaponSystem.cpp
    src/gameplay/DialogueSystem.cpp
    src/gameplay/CreatureAI.cpp
    src/gameplay/FactorySystem.cpp 
    src/video/MaterialSystem.cpp 
    src/core/AssetParser.cpp
    src/gameplay/ClassSystem.cpp
    src/gameplay/VehiclePhysics.cpp
    src/video/Renderer3D.cpp
    src/video/Shader.cpp
)

# Ищем SDL2 в системе
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Платформозависимый код
if(ANDROID)
    message(STATUS "Configuring for Android (API 29-33 / ARM64)")
    add_definitions(-DCENTRALIA_PLATFORM_ANDROID)
    set(PLATFORM_SOURCES src/platform/PlatformAndroid.cpp)
    add_library(${PROJECT_NAME} SHARED ${ENGINE_SOURCES} ${PLATFORM_SOURCES})
    target_link_libraries(${PROJECT_NAME} log glad)
    
elseif(WIN32)
    message(STATUS "Configuring for Windows 10")
    add_definitions(-DCENTRALIA_PLATFORM_WINDOWS)
    set(PLATFORM_SOURCES src/platform/PlatformWin32.cpp)
    
    # Сборка исполняемого файла .exe
    add_executable(${PROJECT_NAME} src/main.cpp ${ENGINE_SOURCES} ${PLATFORM_SOURCES})
    
    # Линкуем Winsock2, SDL2, встроенный GLAD и системные библиотеки OpenGL на Windows 10
    target_link_libraries(${PROJECT_NAME} ws2_32 ${SDL2_LIBRARIES} Opengl32 Glu32 glad)
    
else()
    # Linux бинарник (Arch Linux Target)
    message(STATUS "Configuring for Linux (Arch Linux GCC Target)")
    add_definitions(-DCENTRALIA_PLATFORM_LINUX)
    set(PLATFORM_SOURCES src/platform/PlatformLinux.cpp)
    
    add_executable(${PROJECT_NAME} src/main.cpp ${ENGINE_SOURCES} ${PLATFORM_SOURCES})
    target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES} GL GLU glad)
endif()
