#pragma once
#include <cstdint>
#include <array>
#include "gameplay/Math3D.hpp" // Твой Vector3D из ядра движка

namespace Centralia {

// Структура независимой опоры/ноги шасси Титана
struct TitanLegChassis {
    uint8_t  legIndex;
    Vector3D defaultBodyOffset; // Базовое жесткое смещение лапы относительно центра масс меха
    Vector3D currentIKTarget;   // Вычисленная фантомная точка шага на рельефе карты
    Vector3D currentPosition;   // Текущие мировые 3D-координаты сустава/опоры лапы
    Vector3D springVelocity;    // Скорость амортизатора для ИИК-сглаживания колебаний на CPU
    uint8_t  isGrounded;        // 1 - опора зафиксирована на heightLevel, 0 - перенос ноги в воздухе
};

class ProceduralMotionManager {
private:
    std::array<TitanLegChassis, 4> m_legs; // 4-ногое шасси Titanfall 2
    float m_chassisRoll;
    float m_chassisPitch;
    float m_chassisHeightOffset;

    // Константы ИИК-пружин и затухания амортизаторов для тяжелого веса Титана
    static constexpr float SPRING_STIFFNESS    = 22.0f; // Жесткость подвески ВАЗ-2101 / Меха
    static constexpr float SPRING_DAMPING      = 5.2f;  // Сопротивление амортизатора тряске
    static constexpr float STEP_MAX_THRESHOLD  = 2.1f;  // Предел растяжения лапы до триггера шага
    static constexpr float STEP_HEIGHT_BOUNCE  = 0.6f;  // Синусоидальный подъем ноги в воздухе

public:
    ProceduralMotionManager();
    ~ProceduralMotionManager() = default;

    /**
     * @brief Процедурный обсчет ИИК-подвески шасси Титана.
     * @param deltaTime - Квант времени из аппаратно-адаптивного цикла main.cpp
     * @param titanRootPosition - Текущие 3D координаты центра масс Титана на сцене
     * @param currentHeightLevel - Высотный слой Z-оси (0-52+) из 22-байтовой ячейки test.map под мехом
     */
    void UpdateTitanChassisIK(float deltaTime, const Vector3D& titanRootPosition, float currentHeightLevel);

    // Геттеры для Renderer3D (Modern OpenGL 3.3 Core) под матрицы трансформации шейдеров
    [[nodiscard]] float GetChassisRoll() const noexcept { return m_chassisRoll; }
    [[nodiscard]] float GetChassisPitch() const noexcept { return m_chassisPitch; }
    [[nodiscard]] float GetChassisHeightOffset() const noexcept { return m_chassisHeightOffset; }
    [[nodiscard]] const std::array<TitanLegChassis, 4>& GetTitanLegs() const noexcept { return m_legs; }
};

} // namespace Centralia
