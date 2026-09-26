#define _USE_MATH_DEFINES // Ставим на самую первую строчку для cl.exe
#pragma once
#include <cmath> // Теперь cmath на Windows увидит M_PI

namespace Centralia {

// Структура трехмерного вектора для позиций в мире, как в Fallout
struct Vector3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vector3D() = default;
    Vector3D(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vector3D operator+(const Vector3D& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vector3D operator-(const Vector3D& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vector3D operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    
    float Length() const { return std::sqrt(x*x + y*y + z*z); }
    
    Vector3D Normalize() const {
        float len = Length();
        if (len > 0.0f) return {x / len, y / len, z / len};
        return {0.0f, 0.0f, 0.0f};
    }
};

// Свободная камера от третьего лица (как в State of Decay / Fallout)
class Camera3D {
public:
    Vector3D position;   // Позиция камеры в 3D пространстве
    Vector3D target;     // Куда смотрит 
    float pitch = 0.0f;  
    float yaw = -90.0f;  
    float distanceToPlayer = 5.0f; 
    
    // НАШИ АДМИНСКИЕ МОДИФИКАТОРЫ:
    bool  isAdminMode = false;   // Флаг переключения режима
    float flySpeed = 15.0f;       // Скорость полета админа

    Camera3D() : position(0.0f, 5.0f, -5.0f), target(0.0f, 0.0f, 0.0f) {}

    // 1. Твой оригинальный метод следования за игроком
    void FollowPlayer(const Vector3D& playerPos, float mouseXOffset, float mouseYOffset) {
        if (isAdminMode) return; // Если включен админ-режим, этот обсчет пропускается

        yaw += mouseXOffset;
        pitch += mouseYOffset;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        float pitchRad = pitch * M_PI / 180.0f;
        float yawRad = yaw * M_PI / 180.0f;

        position.x = playerPos.x - distanceToPlayer * std::cos(pitchRad) * std::sin(yawRad);
        position.y = playerPos.y + distanceToPlayer * std::sin(pitchRad) + 1.5f; 
        position.z = playerPos.z - distanceToPlayer * std::cos(pitchRad) * std::cos(yawRad);
        
        target = playerPos;
    }

    // 2. Новый метод свободного полета админа (без привязки к координатам игрока)
    void MoveFreeCam(float forward, float right, float up, float deltaTime) noexcept {
        if (!isAdminMode) return;
        
        position.x += right * flySpeed * deltaTime;
        position.y += up * flySpeed * deltaTime;
        position.z += forward * flySpeed * deltaTime;
        
        target = position + Vector3D(0.0f, 0.0f, 1.0f); // Смотрим строго перед собой
    }
};



} // namespace Centralia
