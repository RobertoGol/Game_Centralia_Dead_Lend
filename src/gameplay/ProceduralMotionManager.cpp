#include "gameplay/ProceduralMotionManager.hpp"
#include "platform/Platform.hpp"
#include <cmath>

namespace Centralia {

ProceduralMotionManager::ProceduralMotionManager() 
    : m_chassisRoll(0.0f), m_chassisPitch(0.0f), m_chassisHeightOffset(0.0f) {
    
    // Жесткая инициализация четырех выносов опор шасси без аллокаций кучи
    m_legs[0] = { 0, { -1.6f, 0.0f,  1.6f }, {0,0,0}, {0,0,0}, {0,0,0}, 1 }; // Передняя Левая
    m_legs[1] = { 1, {  1.6f, 0.0f,  1.6f }, {0,0,0}, {0,0,0}, {0,0,0}, 1 }; // Передняя Правая
    m_legs[2] = { 2, { -1.6f, 0.0f, -1.6f }, {0,0,0}, {0,0,0}, {0,0,0}, 1 }; // Задняя Левая
    m_legs[3] = { 3, {  1.6f, 0.0f, -1.6f }, {0,0,0}, {0,0,0}, {0,0,0}, 1 }; // Задняя Правая

    // Прописываем стартовые позиции лап под нулевую точку
    for (auto& leg : m_legs) {
        leg.currentPosition.x = leg.defaultBodyOffset.x;
        leg.currentPosition.z = leg.defaultBodyOffset.z;
        leg.currentPosition.y = 0.0f;
        leg.currentIKTarget   = leg.currentPosition;
    }
}

void ProceduralMotionManager::UpdateTitanChassisIK(float deltaTime, const Vector3D& titanRootPosition, float currentHeightLevel) {
    float averageLegHeight = 0.0f;

    // Скрипт поочередного шага (чтобы Титан не поднимал все лапы одновременно и не падал)
    bool isAnyLegStepping = false;
    for (const auto& l : m_legs) {
        if (!l.isGrounded) {
            isAnyLegStepping = true;
            break;
        }
    }

    for (auto& leg : m_legs) {
        // 1. Вычисляем идеальную мировую проекцию точки опоры на основе ландшафта test.map
        Vector3D targetWorldRay;
        targetWorldRay.x = titanRootPosition.x + leg.defaultBodyOffset.x;
        targetWorldRay.z = titanRootPosition.z + leg.defaultBodyOffset.z;
        targetWorldRay.y = currentHeightLevel; // Слой Z-оси из бинарной ячейки карты

        float deltaX = targetWorldRay.x - leg.currentPosition.x;
        float deltaZ = targetWorldRay.z - leg.currentPosition.z;
        float groundErrorDist = std::sqrt(deltaX * deltaX + deltaZ * deltaZ);

        // 2. Триггер фазы переноса лапы в воздухе (с защитой от одновременного отрыва ног)
        if (groundErrorDist > STEP_MAX_THRESHOLD && !isAnyLegStepping && leg.isGrounded) {
            leg.isGrounded = 0;
            // Рассчитываем упреждение точки приземления (шаг чуть наперед с запасом)
            leg.currentIKTarget.x = targetWorldRay.x + (titanRootPosition.x - leg.currentPosition.x) * 0.4f;
            leg.currentIKTarget.z = targetWorldRay.z + (titanRootPosition.z - leg.currentPosition.z) * 0.4f;
            leg.currentIKTarget.y = targetWorldRay.y;
            isAnyLegStepping = true; // Блокируем шаг для остальных лап на этом тике CPU
        }

        // 3. Обсчет ИИК-амортизации Hooke's Law (Закон Гука) + Damping для симуляции веса Титана
        if (!leg.isGrounded) {
            // Расчет физических сил сжатия/растяжения ИИК-пружин суставов ног
            float forceX = (leg.currentIKTarget.x - leg.currentPosition.x) * SPRING_STIFFNESS;
            float forceZ = (leg.currentIKTarget.z - leg.currentPosition.z) * SPRING_STIFFNESS;
            
            // Траектория синусоидального подъема колена меха в фазе переноса ноги
            float currentProgress = 1.0f - (GetDistance3D(leg.currentPosition, leg.currentIKTarget) / STEP_MAX_THRESHOLD);
            float arcHeightY = std::sin(currentProgress * 3.14159f) * STEP_HEIGHT_BOUNCE;
            float forceY = ((leg.currentIKTarget.y + arcHeightY) - leg.currentPosition.y) * SPRING_STIFFNESS;

            // Интегрируем силы в вектор ускорения и гасим колебания
            leg.springVelocity.x += (forceX - leg.springVelocity.x * SPRING_DAMPING) * deltaTime;
            leg.springVelocity.y += (forceY - leg.springVelocity.y * SPRING_DAMPING) * deltaTime;
            leg.springVelocity.z += (forceZ - leg.springVelocity.z * SPRING_DAMPING) * deltaTime;

            // Сдвигаем суставы в 3D-пространстве сцены
            leg.currentPosition.x += leg.springVelocity.x * deltaTime;
            leg.currentPosition.y += leg.springVelocity.y * deltaTime;
            leg.currentPosition.z += leg.springVelocity.z * deltaTime;

            // Проверяем фиксацию опоры на грунте (касание)
            if (GetDistance3D(leg.currentPosition, leg.currentIKTarget) < 0.1f) {
                leg.isGrounded = 1;
                leg.springVelocity = {0.0f, 0.0f, 0.0f};
                // Легкая встряска 3D-камеры на CPU при падении тяжелой стопы Титана
                Platform::Log("[TITAN CHASSIS]: Лапа " + std::to_string(leg.legIndex) + " зафиксирована на грунте слоя " + std::to_string((int)currentHeightLevel));
            }
        } else {
            // Нога намертво стоит на ячейке Concrete / Dirt рельефа карты
            leg.currentPosition.x = targetWorldRay.x;
            leg.currentPosition.z = targetWorldRay.z;
            leg.currentPosition.y = targetWorldRay.y;
        }

        averageLegHeight += leg.currentPosition.y;
    }

    // 4. Расчет дифференциала крена (Roll/Pitch) и высоты корпуса меха для шейдера Modern OpenGL
    float flY = m_legs[0].currentPosition.y; // Передняя Левая
    float frY = m_legs[1].currentPosition.y; // Передняя Правая
    float blY = m_legs[2].currentPosition.y; // Задняя Левая
    float brY = m_legs[3].currentPosition.y; // Задняя Правая

    m_chassisHeightOffset = (averageLegHeight / 4.0f) - titanRootPosition.y;
    m_chassisRoll         = ((flY + blY) - (frY + brY)) * 0.12f;  // Боковой наклон наCPU
    m_chassisPitch        = ((flY + frY) - (blY + brY)) * 0.12f;  // Продольный наклон при спуске/подъеме
}

// Статический метод расстояния (изолирован от перегрузок)
float GetDistance3D(const Vector3D& a, const Vector3D& b) {
    return std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2) + std::pow(a.z - b.z, 2));
}

} // namespace Centralia
