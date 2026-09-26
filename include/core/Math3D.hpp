#define _USE_MATH_DEFINES
#pragma once
#include <cmath>

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
    Vector3D target;     // Куда смотрит (обычно на спину игрока)
    float pitch = 0.0f;  // Наклон вверх/вниз
    float yaw = -90.0f;  // Поворот влево/вправо
    float distanceToPlayer = 5.0f; // Дистанция до персонажа

    Camera3D() : position(0.0f, 5.0f, -5.0f), target(0.0f, 0.0f, 0.0f) {}

    // Обновление позиции камеры вокруг игрока на основе ввода мыши/тача
    void FollowPlayer(const Vector3D& playerPos, float mouseXOffset, float mouseYOffset) {
        yaw += mouseXOffset;
        pitch += mouseYOffset;

        // Ограничиваем наклон, чтобы камера не переворачивалась через голову
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        // Пересчитываем координаты камеры в сферической системе вокруг игрока
        float pitchRad = pitch * M_PI / 180.0f;
        float yawRad = yaw * M_PI / 180.0f;

        position.x = playerPos.x - distanceToPlayer * std::cos(pitchRad) * std::sin(yawRad);
        position.y = playerPos.y + distanceToPlayer * std::sin(pitchRad) + 1.5f; // Смещение чуть выше головы
        position.z = playerPos.z - distanceToPlayer * std::cos(pitchRad) * std::cos(yawRad);
        
        target = playerPos;
    }
};

} // namespace Centralia
