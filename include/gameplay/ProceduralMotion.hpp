#pragma once
#include "core/Math3D.hpp"
#include <vector>

namespace Centralia {

// Типы шасси для всей будущей техники и роботов Централии
enum class ChassisType {
    Titan_4_Legged,   // Наш текущий четвероногий Титан
    Titan_Bipedal,    // Гуманоидный робот / Экзоскелет
    Crawler_Multileg, // Ползущая многоногая платформа
    Tracked_Tank      // Гусеничный танк / Бронетехника
};

// Структура отдельной механической ноги с поддержкой Инверсной Кинематики (IK)
struct ProceduralLeg {
    Vector3D hipOffset;       // Точка крепления сустава к корпусу Титана
    Vector3D currentFootPos;   // Текущая позиция ступни на земле в 3D мире
    Vector3D targetFootPos;    // Целевая точка, куда нога планирует наступить
    
    float stepProgress = 1.0f; // Отрезок шага от 0.0f (подъем) до 1.0f (наступил)
    bool isMoving = false;     // Находится ли нога в воздухе прямо сейчас
};

class ProceduralMotionManager {
private:
    ChassisType m_type;
    std::vector<ProceduralLeg> m_legs;
    float m_stepLength = 2.5f;   // Длина шага огромного Титана
    float m_stepHeight = 0.8f;   // Насколько высоко Титан поднимает ногу над грунтом
    float m_stepSpeed = 4.0f;    // Скорость анимации переноса суставов

    ProceduralMotionManager(ChassisType type = ChassisType::Titan_4_Legged); // Синглтон / Конструктор

public:
    static ProceduralMotionManager& GetInstance() {
        static ProceduralMotionManager instance;
        return instance;
    }

    // Инициализация структуры суставов для 4-х ногого Титана
    void InitializeChassis(ChassisType type);

    // Главный математический обсчет походки на CPU (вызывается каждый кадр в Update)
    void UpdateTitanMovement(const Vector3D& bodyPosition, const Vector3D& moveDirection, float deltaTime);

    const std::vector<ProceduralLeg>& GetLegs() const { return m_legs; }
};

} // namespace Centralia
