#pragma once
#include <cmath>
#include <cstdint>
#include "platform/Platform.hpp"

namespace Centralia {

struct Vector2D {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vector2D() noexcept = default;
    constexpr Vector2D(float px, float py) noexcept : x(px), y(py) {}

    [[nodiscard]] float Length() const noexcept { return std::sqrt(x * x + y * y); }
    [[nodiscard]] Vector2D Normalized() const noexcept {
        float len = Length();
        return (len > 0.0f) ? Vector2D(x / len, y / len) : Vector2D(0.0f, 0.0f);
    }
};

struct Vector3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vector3D() noexcept = default;
    constexpr Vector3D(float px, float py, float pz) noexcept : x(px), y(py), z(pz) {}

    [[nodiscard]] float Length() const noexcept { return std::sqrt(x * x + y * y + z * z); }
    [[nodiscard]] float LengthSquared() const noexcept { return x * x + y * y + z * z; }
    
    [[nodiscard]] Vector3D Normalized() const noexcept {
        float len = Length();
        return (len > 0.0f) ? Vector3D(x / len, y / len, z / len) : Vector3D(0.0f, 0.0f, 0.0f);
    }

    [[nodiscard]] static float Dot(const Vector3D& a, const Vector3D& b) noexcept {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    [[nodiscard]] static Vector3D Cross(const Vector3D& a, const Vector3D& b) noexcept {
        return Vector3D(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }

    Vector3D& operator+=(const Vector3D& o) noexcept { x += o.x; y += o.y; z += o.z; return *this; }
    Vector3D& operator-=(const Vector3D& o) noexcept { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vector3D& operator*=(float scalar) noexcept { x *= scalar; y *= scalar; z *= scalar; return *this; }
};

inline Vector3D operator+(Vector3D a, const Vector3D& b) noexcept { return a += b; }
inline Vector3D operator-(Vector3D a, const Vector3D& b) noexcept { return a -= b; }
inline Vector3D operator*(Vector3D a, float scalar) noexcept { return a *= scalar; }
inline Vector3D operator*(float scalar, Vector3D a) noexcept { return a *= scalar; }

struct Matrix4x4 {
    float m[4][4] = {
        {1.0f, 0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 0.0f, 1.0f}
    };

    [[nodiscard]] static Matrix4x4 Identity() noexcept {
        return Matrix4x4{};
    }
};

} // namespace Centralia