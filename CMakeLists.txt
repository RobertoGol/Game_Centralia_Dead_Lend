cmake_minimum_required(VERSION 3.16)
project(Game_Centralia_Dead_Lend VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CENTRALIA_SOURCES
    src/main.cpp
    src/glad.c
    src/core/Engine.cpp
    src/core/LoginSystem.cpp
    src/gameplay/ClassSystem.cpp
    src/gameplay/CraftingManager.cpp
    src/gameplay/CreatureAI.cpp
    src/gameplay/DialogueSystem.cpp
    src/gameplay/FactorySystem.cpp
    src/gameplay/ItemDatabase.cpp
    src/gameplay/MapSystem.cpp
    src/gameplay/ModificationSystem.cpp
    src/gameplay/MonsterAISystem.cpp
    src/gameplay/Player.cpp
    src/gameplay/PowerArmorStateData.cpp
    src/gameplay/ProceduralMotion.cpp
    src/gameplay/ServerShop.cpp
    src/gameplay/VehiclePhysics.cpp
    src/gameplay/WeaponSystem.cpp
)

if(ANDROID)
    add_library(${PROJECT_NAME} SHARED ${CENTRALIA_SOURCES})
else()
    add_executable(${PROJECT_NAME} ${CENTRALIA_SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN _USE_MATH_DEFINES)
    set_source_files_properties(src/glad.c PROPERTIES LANGUAGE C)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
endif()

# ИСПРАВЛЕНО: Настройка путей SDL2
set(SDL2_PATH "I:/SDL2" CACHE PATH "Path to SDL2 VC Development SDK")
target_include_directories(${PROJECT_NAME} PRIVATE "${SDL2_PATH}/include")

if(ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE log)
elseif(WIN32)
    target_link_directories(${PROJECT_NAME} PRIVATE "${SDL2_PATH}/lib/x64")
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 opengl32 glu32 SDL2 SDL2main)
    target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN _USE_MATH_DEFINES NOMINMAX)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE SDL2 GL GLU)
endif()