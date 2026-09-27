cmake_minimum_required(VERSION 3.16)
project(Game_Centralia_Dead_Lend VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. Map our monolithic header-only include directories (.hpp layers)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# 2. Track hardware driver loaders (glad.c is our only non-header asset compilation dependency)
set(ENGINE_SOURCES
    src/glad.c
)

# ========================================================================
# 3. MANUAL SDL2 LINKING SYSTEM (Bypassing package search errors)
# ========================================================================
# CHANGE THIS PATH: Set this to the exact location of your SDL2 folder on your I: or C: drive!
set(SDL2_PATH "I:/SDL2" CACHE PATH "Path to SDL2 VC Development SDK")

# Force explicit header and static library searches
include_directories("${SDL2_PATH}/include")

if(WIN32)
    # Map paths to standard 64-bit architecture lib files
    link_directories("${SDL2_PATH}/lib/x64")
endif()

# --- NATIVE OVERRIDE COMPILATION MATRIX BRANCHING ---

if(ANDROID)
    message(STATUS "[BUILD]: Configuring dynamic library compilation hooks for Android NDK")
    add_library(${PROJECT_NAME} SHARED src/main.cpp ${ENGINE_SOURCES})
    target_link_libraries(${PROJECT_NAME} log)
    
elseif(WIN32)
    message(STATUS "[BUILD]: Configuring native executable layout hooks for Windows 10/11 Host OS")
    add_compile_definitions(_USE_MATH_DEFINES)
    
    # ИСПРАВЛЕНО ТОЧЕЧНО: Флаг /Za заменен на /Ze для совместимости Windows Kits с Си-компилятором
    set_source_files_properties(src/glad.c PROPERTIES 
        LANGUAGE C 
        COMPILE_FLAGS "/Y- /Ze /TC"
    )
    
    # Generate our direct executable build target binary
    add_executable(${PROJECT_NAME} src/main.cpp ${ENGINE_SOURCES})
    
    # Explicitly link native libraries: ws2_32 (network), opengl32, and raw SDL2 files
    target_link_libraries(${PROJECT_NAME} 
        ws2_32 
        opengl32 
        glu32
        SDL2
        SDL2main
    )

else()
    message(STATUS "[BUILD]: Configuring build target properties for native Linux platform")
    add_executable(${PROJECT_NAME} src/main.cpp ${ENGINE_SOURCES})
    target_link_libraries(${PROJECT_NAME} SDL2 GL GLU)
endif()
