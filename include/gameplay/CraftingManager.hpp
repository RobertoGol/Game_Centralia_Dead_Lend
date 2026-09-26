#pragma once
#include <cstdint>
#include <array>
#include "gameplay/ItemDatabase.hpp"
#include "gameplay/FactorySystem.hpp"

namespace Centralia {

#pragma pack(push, 1)
// Плотная структура рецепта крафта по чертежу (State of Decay 2)
struct BlueprintRecord {
    uint32_t     blueprintId;
    uint32_t     targetItemId;          // ID предмета, который скрафтим (из ItemDatabase)
    uint32_t     requiredItemCount;      // Сколько штук получим на выходе
    
    // Ресурсы, необходимые для сборки (тратятся из FactorySystem)
    float        requiredIronScrap;      // Требуемый металлолом
    float        requiredTechMods;       // Требуемые компоненты электроники
    uint32_t     requiredToolId;         // ID инструмента в инвентаре (0, если не нужен)
};
#pragma pack(pop)

class CraftingManager {
private:
    static constexpr size_t MAX_BLUEPRINTS = 128;
    std::array<BlueprintRecord, MAX_BLUEPRINTS> m_blueprintRegistry;
    uint32_t m_blueprintCount;

public:
    CraftingManager() noexcept;
    ~CraftingManager() = default;

    /**
     * @brief Инициализация чертежей крафта (пушки Titanfall, пластины T-60, медицина).
     */
    void InitializeBlueprints() noexcept;

    /**
     * @brief Проверка возможности крафта и сборка предмета по чертежу.
     * @param blueprintId - ID выбранного чертежа
     * @param player - Ссылка на игрока для проверки инвентаря и выдачи лута
     * @param factoryContext - Твоя фабричная энергосистема для списания ресурсов
     * @param itemDb - Ссылка на базу предметов для валидации ТТХ
     */
    bool TryExecuteCraft(uint32_t blueprintId, Player& player, FactoryEngineContext& factoryContext, const ItemDatabase& itemDb) noexcept;

    // Быстрый поиск рецепта по ID
    [[nodiscard]] const BlueprintRecord* GetBlueprint(uint32_t blueprintId) const noexcept;
};

} // namespace Centralia
