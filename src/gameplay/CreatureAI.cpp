#include "gameplay/CreatureAI.hpp"
#include "platform/Platform.hpp"
#include "core/MemoryManager.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Centralia {

Creature::Creature(uint32_t id, const std::string& name, CreatureSpecies species)
    : m_id(id), m_name(name), m_species(species), m_position(0.0f, 0.0f, 0.0f) 
{
    // Настраиваем покомпонентные профили анатомии под каждый вид индивидуально
    m_anatomyProfile[AnatomyPart::Head_Sensor]        = { "Голова/Сенсоры", 50.0f, 50.0f, false };
    m_anatomyProfile[AnatomyPart::Torso_Chassis]      = { "Корпус/Торс", 150.0f, 150.0f, false };
    m_anatomyProfile[AnatomyPart::Left_Leg_Hydraulic]  = { "Левая Опора", 80.0f, 80.0f, false };
    m_anatomyProfile[AnatomyPart::Right_Leg_Hydraulic] = { "Правая Опора", 80.0f, 80.0f, false };
    m_anatomyProfile[AnatomyPart::Weapon_System_Link]  = { "Оружейный Узел", 70.0f, 70.0f, false };

    // Задаем лимиты прочности (ХП) в зависимости от природы и вида существа
    if (m_species == CreatureSpecies::Mutant_Behemoth) {
        m_totalHealth = 1500.0f; // Огромное ХП босса-Бегемота с твоей гифки
        m_anatomyProfile[AnatomyPart::Torso_Chassis].health = 1000.0f;
        m_anatomyProfile[AnatomyPart::Torso_Chassis].maxHealth = 1000.0f;
    } else if (m_species == CreatureSpecies::Mechanical_Mecha) {
        m_totalHealth = 800.0f;  // Прочность тяжелого шагающего Робота/Титана
    } else {
        m_totalHealth = 100.0f;  // Стандартный органический человек или гуль
    }
}

void Creature::ApplyComponentDamage(AnatomyPart part, float rawDamage) {
    auto it = m_anatomyProfile.find(part);
    if (it == m_anatomyProfile.end() || it->second.isSeveredOrDestroyed) return;

    AnatomyComponent& comp = it->second;
    float finalDmg = rawDamage;

    // --- РАЗДЕЛЕНИЕ ЛОГИКИ ПОВРЕЖДЕНИЙ ПО АНАТОМИЧЕСКИМ ВИДАМ ---
    
    // 1. Специфика РОБОТОВ И МЕХОВ: Броня гасит урон, уязвимость у сенсоров
    if (m_species == CreatureSpecies::Mechanical_Mecha) {
        if (part == AnatomyPart::Torso_Chassis) {
            finalDmg -= 20.0f; // Листы титановой брони поглощают 23 единицы урона пули
            if (finalDmg < 0.0f) finalDmg = 0.0f;
        }
        if (part == AnatomyPart::Head_Sensor) {
            finalDmg *= 2.5f; // Попадание в сенсорный блок наносит х2.5 критического урона
            Platform::Log("[COMBAT]: Механический узел: Критический урон по сенсорам Робота!");
        }
    }
    // 2. Специфика МУТАНТОВ-БЕГЕМОТОВ: Защита от критического урона
    else if (m_species == CreatureSpecies::Mutant_Behemoth) {
        if (part == AnatomyPart::Head_Sensor) {
            finalDmg *= 1.2f; // Огромный череп защищен костью, критический множитель срезан до х1.2
        }
    }

    // Наносим вычисленный урон компоненту и общему телу
    comp.health -= finalDmg;
    m_totalHealth -= finalDmg;

    // Физика отсечения / полного уничтожения детали (Расчлененка из твоей задумки LDoE)
    if (comp.health <= 0.0f) {
        comp.health = 0.0f;
        comp.isSeveredOrDestroyed = true;
        
        // РЕАКЦИЯ НА ОТСТРЕЛ КОМПОНЕНТОВ:
        if (m_species == CreatureSpecies::Mechanical_Mecha && part == AnatomyPart::Weapon_System_Link) {
            Platform::Log("[COMBAT]: Оружейная шина повреждена. Заплечное орудие робота полностью ОТСТРЕЛЕНО!");
        }
        
        if (m_species == CreatureSpecies::Mutant_Behemoth && 
            (part == AnatomyPart::Left_Leg_Hydraulic || part == AnatomyPart::Right_Leg_Hydraulic)) 
        {
            m_isEnraged = true; // Раненый в лапы Бегемот свирепеет (Enrage State), компенсируя скорость уроном
            Platform::Log("[COMBAT]: Лапа Бегемота пробита! Монстр впал в состояние боевой ЯРОСТИ.");
        }

        if (m_species == CreatureSpecies::Rad_Ghoul && part == AnatomyPart::Head_Sensor) {
            m_totalHealth = 0.0f; // Мгновенная смерть гуля при уничтожении головы
            Platform::Log("[COMBAT]: Голова радиоактивного гуля взорвана точным выстрелом в 3D!");
        }
    }
}

void Creature::UpdateAiTick(float deltaTime, const Vector3D& playerPos, MemoryManager& memory) {
    // Проверка смерти существа
    if (m_totalHealth <= 0.0f) {
        // ЭФФЕКТ ДЕТОНАЦИИ РАДИОАКТИВНЫХ ГУЛЕЙ:
        if (m_species == CreatureSpecies::Rad_Ghoul) {
            Vector3D delta = m_position - playerPos;
            if (delta.Length() < 6.0f) { // Радиус поражения выброса изотопов — 6 метров в 3D
                int32_t currentRad = memory.GetRegistryValue("player_radiation");
                memory.SetRegistryValue("player_radiation", currentRad + 25); // Лупит +25 рад по игроку
                Platform::Log("[AI EVENT]: Радиоактивный гуль детонировал! Игрок облучен радиоактивной пылью.");
            }
            // Меняем вид на органику, чтобы предотвратить повторный круговой обсчет взрыва трупа на CPU
            m_species = CreatureSpecies::Humanoid_Organic; 
        }
        return;
    }

    // ХАРДКОРНЫЙ ОБСЧЕТ ХРОМОТЫ: Скорость ИИ падает, если повреждены ноги/приводы шасси
    float movementSpeed = 3.5f; // Базовая скорость бега мобов
    
    // БЕЗОПАСНАЯ ИСПРАВЛЕННАЯ ПРОВЕРКА ФЛАГОВ РАСЧЛЕНЕНКИ (Убрали некорректный метод isOriginal)
    if (m_anatomyProfile[AnatomyPart::Left_Leg_Hydraulic].isSeveredOrDestroyed || 
        m_anatomyProfile[AnatomyPart::Right_Leg_Hydraulic].isSeveredOrDestroyed) 
    {
        movementSpeed = 1.0f; // Отстрел опоры заставляет ИИ хромать/ползти на скорости 1.0
    }

    // Модификатор ярости Бегемота (Монстр ускоряется, игнорируя боль)
    if (m_isEnraged) {
        movementSpeed *= 1.8f; 
    }

    // Процессор симулирует вектор погони ИИ за координатами игрока в 3D-пространстве
    Vector3D aiMoveDir = playerPos - m_position;
    float currentDistance = aiMoveDir.Length();

    // Зона обнаружения и агра существа (срабатывает в пределах 30 метров)
    if (currentDistance < 30.0f && currentDistance > 1.3f) {
        m_position = m_position + (aiMoveDir.Normalize() * movementSpeed * deltaTime);
    }
}

} // namespace Centralia
