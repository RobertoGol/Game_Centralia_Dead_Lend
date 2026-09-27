#pragma once
#include "gameplay/Player.hpp" // Предоставляет типы ItemType для бесшовной интеграции
#include "gameplay/ModificationSystem.hpp" // Предоставляет типы VehicleModification и UniverseItem
#include <string>
#include <unordered_map>
#include <cstdint>

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
    
    // Внутренние специализированные хранилища расширенных ТТХ
    std::unordered_map<uint32_t, VehicleModification> m_vehicleMods;
    std::unordered_map<uint32_t, UniverseItem> m_loreItems;

    ItemDatabase(); // Приватный конструктор синглтона

public:
    ~ItemDatabase() = default;

    // Запрещаем копирование синглтона
    ItemDatabase(const ItemDatabase&) = delete;
    ItemDatabase& operator=(const ItemDatabase&) = delete;

    static ItemDatabase& GetInstance() {
        static ItemDatabase instance;
        return instance;
    }

    // Правильный константный метод поиска для CraftingManager, возвращающий указатель
    [[nodiscard]] const ItemTemplate* GetItemTemplatePtr(uint32_t itemId) const noexcept {
        auto it = m_templates.find(itemId);
        if (it != m_templates.end()) {
            return &it->second;
        }
        return nullptr;
    }

    // Инициализация дефолтного лута (Fallout / State of Decay сеттинг)
    void Initialize();

    // Поиск шаблона предмета по ID с копированием в выходную структуру
    bool GetTemplate(uint32_t id, ItemTemplate& outTemplate) const;

    // Дополнительные геттеры расширенных модификаций для автофизики
    [[nodiscard]] const std::unordered_map<uint32_t, VehicleModification>& GetVehicleMods() const noexcept { return m_vehicleMods; }
    [[nodiscard]] const std::unordered_map<uint32_t, UniverseItem>& GetLoreItems() const noexcept { return m_loreItems; }
};

} // namespace Centralia
