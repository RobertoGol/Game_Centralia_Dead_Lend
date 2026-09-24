#pragma once
#include "gameplay/Player.hpp" // Берем типы ItemType из класса игрока
#include <string>
#include <unordered_map>

namespace Centralia {

// Специфические боевые и выживальческие параметры предмета
struct ItemTemplate {
    uint32_t id;
    std::string name;
    std::string description;
    ItemType type;
    float weight;
    uint16_t max_stack;
    
    // Эффекты применения предметов
    float combat_damage = 0.0f;     // Урон (для оружия)
    float damage_resistance = 0.0f; // Защита от физического урона (для брони)
    float rad_resistance = 0.0f;    // Защита от радиации (для противогазов/костюмов)
    
    float heal_amount = 0.0f;       // Сколько ХП восстанавливает (для аптечек)
    float rad_remedy = 0.0f;        // Сколько рад выводит (для антирадина)
};

class ItemDatabase {
private:
    std::unordered_map<uint32_t, ItemTemplate> m_templates;

    ItemDatabase(); // Синглтон: база данных в памяти должна быть в одном экземпляре

public:
    static ItemDatabase& GetInstance() {
        static ItemDatabase instance;
        return instance;
    }

    // Инициализация дефолтного лута (Fallout / State of Decay сеттинг)
    void Initialize();

    // Поиск шаблона предмета по ID
    bool GetTemplate(uint32_t id, ItemTemplate& outTemplate) const;
};

} // namespace Centralia
