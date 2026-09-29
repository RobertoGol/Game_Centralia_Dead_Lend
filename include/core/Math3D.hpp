#pragma once
#define _USE_MATH_DEFINES 
#include <cmath>          
#include <cstdint>        

namespace Centralia {

enum class ActiveControlMode : int32_t {
    Standard_Player = 0,
    Admin_Observer  = 1
};

enum class EngineControlMode : int32_t {
    Standard_Player = 0,
    Admin_Observer  = 1
};

struct Vector3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    inline Vector3D()   = default;
    inline Vector3D(float _x, float _y, float _z)   : x(_x), y(_y), z(_z) {};

    inline Vector3D operator+(const Vector3D& other) const   { 
        return { x + other.x, y + other.y, z + other.z }; 
    };
    
    inline Vector3D operator-(const Vector3D& other) const   { 
        return { x - other.x, y - other.y, z - other.z }; 
    };
    
    inline Vector3D operator*(float scalar) const   { 
        return { x * scalar, y * scalar, z * scalar }; 
    };
    
    [[nodiscard]] inline Vector3D Cross(const Vector3D& other) const   {
        return {
            y * other.z - z * other.y,
            z * other.x - x * other.z,
            x * other.y - y * other.x
        };
    };

    [[nodiscard]] inline float Dot(const Vector3D& other) const   {
        return x * other.x + y * other.y + z * other.z;
    };
    
    [[nodiscard]] inline float Length() const   { 
        return std::sqrt(x * x + y * y + z * z); 
    };
    
    [[nodiscard]] inline Vector3D Normalize() const   {
        float len = Length();
        if (len > 0.0f) return { x / len, y / len, z / len };
        return { 0.0f, 0.0f, 0.0f };
    };
};

class Camera3D {
public:
    Vector3D position;        
    Vector3D target;          
    float pitch = 0.0f;  
    float yaw = -90.0f;  
    float distanceToPlayer = 5.0f; 
    bool  isAdminMode = false; 
    float flySpeed = 15.0f;    

    inline Camera3D()   : position(0.0f, 5.0f, -5.0f), target(0.0f, 0.0f, 0.0f) {};

    inline void FollowPlayer(const Vector3D& playerPos, float mouseXOffset, float mouseYOffset)   {
        if (isAdminMode) return;

        yaw += mouseXOffset;
        pitch += mouseYOffset;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        float pitchRad = pitch * static_cast<float>(M_PI) / 180.0f;
        float yawRad = yaw * static_cast<float>(M_PI) / 180.0f;

        position.x = playerPos.x - distanceToPlayer * std::cos(pitchRad) * std::sin(yawRad);
        position.y = playerPos.y + distanceToPlayer * std::sin(pitchRad) + 1.5f; 
        position.z = playerPos.z - distanceToPlayer * std::cos(pitchRad) * std::cos(yawRad);
        
        target = playerPos;
    };

    inline void MoveFreeCam(float forward, float right, float up, float deltaTime)   {
        if (!isAdminMode) return;
        
        position.x += right * flySpeed * deltaTime;
        position.y += up * flySpeed * deltaTime;
        position.z += forward * flySpeed * deltaTime;
        
        target = position + Vector3D(0.0f, 0.0f, 1.0f); 
    };
};

} // namespace Centralia
