#include "gameplay/ProceduralMotion.hpp"
#include "platform/Platform.hpp"
#include <cmath>

namespace Centralia {

ProceduralMotionManager::ProceduralMotionManager(ChassisType type) {
    InitializeChassis(type);
}

void ProceduralMotionManager::InitializeChassis(ChassisType type) {
    m_type = type;
    m_legs.clear();

    if (m_type == ChassisType::Titan_4_Legged) {
        m_legs.resize(4);
        // Задаем точки крепления 4-х суставов относительно центра корпуса Титана
        m_legs[0].hipOffset = Vector3D(-1.5f, 0.0f,  2.0f);  // Передняя левая
        m_legs[1].hipOffset = Vector3D( 1.5f, 0.0f,  2.0f);  // Передняя правая
        m_legs[2].hipOffset = Vector3D(-1.5f, 0.0f, -2.0f);  // Задняя левая
        m_legs[3].hipOffset = Vector3D( 1.5f, 0.0f, -2.0f);  // Задняя правая

        // Инициализируем стартовые позиции ног на земле
        for (auto& leg : m_legs) {
            leg.currentFootPos = leg.hipOffset; // Ноги стоят ровно под суставами
            leg.targetFootPos = leg.hipOffset;
            leg.stepProgress = 1.0f;
            leg.isMoving = false;
        }
        Platform::Log("ProceduralMotion: 4-Legged Titan chassis joints fully mapped to CPU memory.");
    }
}

void ProceduralMotionManager::UpdateTitanMovement(const Vector3D& bodyPosition, const Vector3D& moveDirection, float deltaTime) {
    if (m_type != ChassisType::Titan_4_Legged) return;

    // Рассчитываем идеальные точки на грунте под суставами во время ходьбы
    for (size_t i = 0; i < m_legs.size(); ++i) {
        ProceduralLeg& leg = m_legs[i];
        
        // Мировая координата сустава на текущем кадре
        Vector3D worldHipPos = bodyPosition + leg.hipOffset;

        // Высчитываем, насколько далеко нога отстала от своего сустава из-за движения корпуса
        Vector3D delta = leg.currentFootPos - worldHipPos;
        float distance = delta.Length();

        // Если корпус ушел далеко вперед, а нога не в воздухе — отдаем приказ сделать шаг
        if (distance > m_stepLength && !leg.isMoving) {
            // Чтобы Титан не подпрыгивал, левая передняя и правая задняя ноги шагают одновременно, 
            // пока остальные две намертво зафиксированы на земле (держим баланс веса массы)
            bool canShareStep = true;
            if (i == 0 || i == 3) {
                if (m_legs[1].isMoving || m_legs[2].isMoving) canShareStep = false;
            } else {
                if (m_legs[0].isMoving || m_legs[3].isMoving) canShareStep = false;
            }

            if (canShareStep) {
                leg.isMoving = true;
                leg.stepProgress = 0.0f;
                // Прогнозируем точку натупления наперед по вектору движения
                leg.targetFootPos = worldHipPos + (moveDirection.Normalize() * m_stepLength);
            }
        }

        // Если нога совершает шаг, плавно двигаем ее суставы по синусоидальной дуге
        if (leg.isMoving) {
            leg.stepProgress += m_stepSpeed * deltaTime;
            if (leg.stepProgress >= 1.0f) {
                leg.stepProgress = 1.0f;
                leg.currentFootPos = leg.targetFootPos;
                leg.isMoving = false; // Шаг завершен, нога коснулась земли Пустоши
            } else {
                // Интерполяция пути по горизонтали (X, Z)
                Vector3D progressVector = leg.targetFootPos - leg.currentFootPos;
                Vector3D currentHorizontal = leg.currentFootPos + (progressVector * leg.stepProgress);
                
                // Магия процедурной анимации: вычисляем высоту подъема Y через синус
                // sin(0) = 0, sin(PI/2) = 1 (пик подъема ноги), sin(PI) = 0 (наступание)
                float wave = std::sin(leg.stepProgress * M_PI);
                currentHorizontal.y = bodyPosition.y + (wave * m_stepHeight);
                
                // Применяем вычисленную 3D позицию к ступне
                leg.currentFootPos = currentHorizontal;
            }
        }
    }
}

} // namespace Centralia
