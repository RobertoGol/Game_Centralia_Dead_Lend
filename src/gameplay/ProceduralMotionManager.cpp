#include "gameplay/ProceduralMotion.hpp"
#include "platform/Platform.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Centralia {

// ==========================================
// ЧАСТЬ 1: ИНВЕРСНАЯ КИНЕМАТИКА (FABRIK ALGORITHM)
// ==========================================

ProceduralMotion::ProceduralMotion() : m_totalReach(0.0f) {}

ProceduralMotion::~ProceduralMotion() {
    m_nodes.clear();
}

void ProceduralMotion::AddNode(const Vector3D& pos, float length) {
    IKChainNode node;
    node.position = pos;
    node.length = length;
    node.rotationLimit = 0.0f;
    m_nodes.push_back(node);
    
    // Пересчитываем общую длину досягаемости кинематической цепи
    m_totalReach = 0.0f;
    for (size_t i = 1; i < m_nodes.size(); ++i) {
        m_totalReach += m_nodes[i].length;
    }
}

bool ProceduralMotion::SolveIK(const Vector3D& targetPosition) {
    if (m_nodes.empty()) return false;

    size_t nodeCount = m_nodes.size();
    Vector3D rootPosition = m_nodes[0].position;

    // Вектор от корня до цели
    Vector3D rootToTarget = targetPosition - rootPosition;
    float targetDistSq = rootToTarget.LengthSquared();

    // Если цель полностью вне досягаемости цепи — вытягиваем суставы по направлению к ней
    if (targetDistSq > (m_totalReach * m_totalReach)) {
        Vector3D direction = rootToTarget.Normalized();
        float accumulatedLength = 0.0f;
        
        for (size_t i = 0; i < nodeCount - 1; ++i) {
            accumulatedLength += m_nodes[i + 1].length;
            m_nodes[i + 1].position = m_nodes[i].position + direction * m_nodes[i + 1].length;
        }
        return false; 
    }

    // Допуски для итеративного схождения FABRIK
    const int maxIterations = 20;
    const float tolerance = 0.0005f;

    Vector3D endEffectorPos = m_nodes[nodeCount - 1].position;
    if ((endEffectorPos - targetPosition).LengthSquared() < (tolerance * tolerance)) {
        return true;
    }

    // Буфер временных позиций суставов в памяти
    std::vector<Vector3D> workingPositions(nodeCount);
    for (size_t i = 0; i < nodeCount; ++i) {
        workingPositions[i] = m_nodes[i].position;
    }

    // Итерационный цикл FABRIK (Forward-Backward Reaching)
    for (int iter = 0; iter < maxIterations; ++iter) {
        // 1. Прямой проход (Backward): тянем от кончика к корневому суставу
        workingPositions[nodeCount - 1] = targetPosition;
        
        for (int i = static_cast<int>(nodeCount) - 2; i >= 0; --i) {
            Vector3D dir = (workingPositions[i] - workingPositions[i + 1]).Normalized();
            workingPositions[i] = workingPositions[i + 1] + dir * m_nodes[i + 1].length;
        }

        // 2. Обратный проход (Forward): фиксируем корень и вытягиваем цепь обратно
        workingPositions[0] = rootPosition;
        
        for (size_t i = 0; i < nodeCount - 1; ++i) {
            Vector3D dir = (workingPositions[i + 1] - workingPositions[i]).Normalized();
            workingPositions[i + 1] = workingPositions[i] + dir * m_nodes[i + 1].length;
        }

        // Проверка погрешности на текущей итерации
        if ((workingPositions[nodeCount - 1] - targetPosition).LengthSquared() < (tolerance * tolerance)) {
            break;
        }
    }

    // Переносим рассчитанные позиции в структуру узлов
    for (size_t i = 0; i < nodeCount; ++i) {
        m_nodes[i].position = workingPositions[i];
    }

    return true;
}

// ==========================================
// ЧАСТЬ 2: МЕНЕДЖЕР ПРОЦЕДУРНОГО ШАССИ ТИТАНОВ И ТЕХНИКИ
// ==========================================

ProceduralMotionManager::ProceduralMotionManager(ChassisType type) 
    : m_type(type), m_stepLength(1.8f), m_stepHeight(0.6f), m_stepSpeed(6.5f) {
    InitializeChassis(type);
}

void ProceduralMotionManager::InitializeChassis(ChassisType type) {
    m_type = type;
    m_legs.clear();

    if (m_type == ChassisType::Titan_4_Legged) {
        m_legs.resize(4);
        
        // Матрица смещения суставов конечностей от центра масс корпуса бронемашины (X, Y, Z)
        m_legs[0].hipOffset = Vector3D(-1.8f, 0.0f,  2.2f);  // Передняя левая
        m_legs[1].hipOffset = Vector3D( 1.8f, 0.0f,  2.2f);  // Передняя правая
        m_legs[2].hipOffset = Vector3D(-1.8f, 0.0f, -2.2f);  // Задняя левая
        m_legs[3].hipOffset = Vector3D( 1.8f, 0.0f, -2.2f);  // Задняя правая

        // Инициализируем стартовые опорные точки ступней под суставами на грунте
        for (size_t i = 0; i < m_legs.size(); ++i) {
            auto& leg = m_legs[i];
            leg.hipOffset = m_legs[i].hipOffset;
            leg.currentFootPos = leg.hipOffset;
            leg.targetFootPos = leg.hipOffset;
            leg.startFootPos = leg.hipOffset;
            leg.stepProgress = 1.0f;
            leg.isMoving = false;
        }

        Platform::Log("ProceduralMotionManager: 4-Legged Titan Chassis successfully configured with full FABRIK bindings.");
    } 
    else if (m_type == ChassisType::Biped_Humanoid) {
        m_legs.resize(2);
        m_legs[0].hipOffset = Vector3D(-0.6f, 0.0f, 0.2f); // Левая нога
        m_legs[1].hipOffset = Vector3D( 0.6f, 0.0f, 0.2f); // Правая нога

        for (auto& leg : m_legs) {
            leg.currentFootPos = leg.hipOffset;
            leg.targetFootPos = leg.hipOffset;
            leg.startFootPos = leg.hipOffset;
            leg.stepProgress = 1.0f;
            leg.isMoving = false;
        }
        Platform::Log("ProceduralMotionManager: Biped Humanoid chassis initialized.");
    }
}

void ProceduralMotionManager::UpdateTitanMovement(const Vector3D& bodyPosition, const Vector3D& moveDirection, float deltaTime) {
    if (m_legs.empty()) return;

    float moveVelocity = moveDirection.Length();
    bool isMovingDirectional = (moveVelocity > 0.01f);

    for (size_t i = 0; i < m_legs.size(); ++i) {
        ProceduralLeg& leg = m_legs[i];

        // Абсолютная мировая координата сустава конечности в текущем кадре
        Vector3D worldHipPos = bodyPosition + leg.hipOffset;

        // Если лапа не шагает и корпус сместился — рассчитываем необходимость нового шага
        if (!leg.isMoving && isMovingDirectional) {
            Vector3D idealTargetPos = worldHipPos + (moveDirection.Normalized() * m_stepLength);
            Vector3D deltaVector = idealTargetPos - leg.currentFootPos;

            // Паттерн походки: для 4-х лап шагаем диагональными парами для сохранения баланса центра масс
            bool gaitPermission = true;
            if (m_legs.size() == 4) {
                for (size_t j = 0; j < m_legs.size(); ++j) {
                    if (i != j && m_legs[j].isMoving && (j % 2 == (i % 2))) {
                        gaitPermission = false; // Диагональный партнер уже в полете, ждем фиксации
                    }
                }
            }

            // Если отклонение превысило допустимый порог и походка разрешает шаг — отрываем лапу
            if (deltaVector.Length() > (m_stepLength * 0.6f) && gaitPermission) {
                leg.startFootPos = leg.currentFootPos; // Жестко фиксируем исходную точку отрыва
                leg.targetFootPos = idealTargetPos;
                leg.stepProgress = 0.0f;
                leg.isMoving = true;
            }
        }

        // Обработка фазы полета/шага лапы в воздухе
        if (leg.isMoving) {
            leg.stepProgress += m_stepSpeed * deltaTime;
            
            if (leg.stepProgress >= 1.0f) {
                leg.stepProgress = 1.0f;
                leg.currentFootPos = leg.targetFootPos;
                leg.startFootPos = leg.targetFootPos; 
                leg.isMoving = false; // Шаг завершен, лапа снова надежно закреплена на грунте
            } else {
                // Синусоидальная параболическая дуга для подъема лапы над препятствиями Пустоши
                float heightArc = std::sin(leg.stepProgress * 3.14159265f) * m_stepHeight;
                
                // Линейная интерполяция по плоскости XZ от точки старта до цели
                Vector3D interpolatedPlane = leg.startFootPos + (leg.targetFootPos - leg.startFootPos) * leg.stepProgress;
                
                // Итоговая 3D позиция сустава с учетом высоты подъема дуги
                leg.currentFootPos = Vector3D(interpolatedPlane.x, interpolatedPlane.y + heightArc, interpolatedPlane.z);
            }
        }
    }
}

} // namespace Centralia