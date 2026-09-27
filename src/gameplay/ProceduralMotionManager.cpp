#include "video/ProceduralMotionManager.hpp"
#include "platform/Platform.hpp"
#include <cmath>

namespace Centralia {

// Конструктор по умолчанию вызывает автоматическую сборку 4-х ногой платформы
ProceduralMotionManager::ProceduralMotionManager() {
    InitializeChassis(ChassisType::Titan_4_Legged);
}

void ProceduralMotionManager::InitializeChassis(ChassisType type) {
    m_type = type;
    m_legs.clear();

    if (m_type == ChassisType::Titan_4_Legged) {
        m_legs.resize(4);
        
        // Матрица смещения суставов от центра масс корпуса (X, Y, Z)
        m_legs[0].hipOffset = Vector3D(-1.5f, 0.0f,  2.0f);  // Передняя левая
        m_legs[1].hipOffset = Vector3D( 1.5f, 0.0f,  2.0f);  // Передняя правая
        m_legs[2].hipOffset = Vector3D(-1.5f, 0.0f, -2.0f);  // Задняя левая
        m_legs[3].hipOffset = Vector3D( 1.5f, 0.0f, -2.0f);  // Задняя правая

        // Инициализируем ступни в дефолтных точках под суставами
        for (auto& leg : m_legs) {
            leg.currentFootPos = leg.hipOffset;
            leg.targetFootPos = leg.hipOffset;
            leg.startFootPos = leg.hipOffset; // Фиксируем стартовый вектор
            leg.stepProgress = 1.0f;
            leg.isMoving = false;
        }
        
        Platform::Log("ProceduralMotion: Четвероногий Титан инициализирован. Баг рекурсии LERP ликвидирован.");
    }
}

void ProceduralMotionManager::UpdateTitanMovement(const Vector3D& bodyPosition, const Vector3D& moveDirection, float deltaTime) {
    if (m_legs.empty()) return;

    float moveVelocity = moveDirection.Length();
    bool isMovingDirectional = (moveVelocity > 0.01f);

    for (size_t i = 0; i < m_legs.size(); ++i) {
        ProceduralLeg& leg = m_legs[i];

        // Идеальное абсолютное положение сустава ноги в 3D пространстве сцены
        Vector3D worldHipPos = bodyPosition + leg.hipOffset;

        if (!leg.isMoving && isMovingDirectional) {
            // Рассчитываем, куда ступня должна наступить с учетом вектора WASD движения
            Vector3D idealTargetPos = worldHipPos + (moveDirection.Normalize() * m_stepLength);
            
            // Вычисляем расстояние от текущей ступни на земле до идеальной точки шага
            Vector3D deltaVector = idealTargetPos - leg.currentFootPos;
            
            // Защита от рассинхронизации: ноги шагают поочередно (диагональный паттерн походки)
            bool otherLegsAnchored = true;
            
            for (size_t j = 0; j < m_legs.size(); ++j) {
                if (i != j && m_legs[j].isMoving && (j % 2 == (i % 2))) {
                    otherLegsAnchored = false;
                }
            }

            if (deltaVector.Length() > m_stepLength * 0.5f && otherLegsAnchored) {
                leg.startFootPos = leg.currentFootPos; // ИСПРАВЛЕНО: Запоминаем точку земли ПЕРЕД началом шага
                leg.targetFootPos = idealTargetPos;
                leg.stepProgress = 0.0f;
                leg.isMoving = true;
            }
        }

        if (leg.isMoving) {
            leg.stepProgress += m_stepSpeed * deltaTime;
            if (leg.stepProgress >= 1.0f) {
                leg.stepProgress = 1.0f;
                leg.currentFootPos = leg.targetFootPos;
                leg.startFootPos = leg.targetFootPos; // Фиксируем новую опору
                leg.isMoving = false;
            } else {
                // Математическая параболическая функция (Синус) для плавного подъема лапы робота в воздух
                float heightArc = std::sin(leg.stepProgress * 3.14159265f) * m_stepHeight;
                
                // ИСПРАВЛЕНО: Интерполяция теперь жестко идет от startFootPos, а не от уплывающейcurrentFootPos
                Vector3D currentPlanePos = leg.startFootPos + (leg.targetFootPos - leg.startFootPos) * leg.stepProgress;
                
                // Итоговая стабильная 3D позиция сустава с учетом высоты подъема
                leg.currentFootPos = Vector3D(currentPlanePos.x, currentPlanePos.y + heightArc, currentPlanePos.z);
            }
        }
    }
}

} // namespace Centralia
