#include "gameplay/CraftingManager.hpp"
#include "gameplay/FactorySystem.hpp"     // Раскрываем полную структуру фабрики для фикса C2027
#include "gameplay/Player.hpp"            // Раскрываем методы игрока
#include "gameplay/ItemDatabase.hpp"       // Раскрываем базу предметов
#include "platform/Platform.hpp"
#include <cstring>
#include <string>

namespace Centralia {

CraftingManager::CraftingManager() noexcept : m_blueprintCount(0) {
    std::memset(m_blueprintRegistry.data(), 0, m_blueprintRegistry.size() * sizeof(BlueprintRecord));
}

void CraftingManager::InitializeBlueprints() noexcept {
    m_blueprintCount = 0;

    // Рецепт 1: Крафт патронов или кустарного мушкета "Log Horizon" из металлолома
    {
        BlueprintRecord& bp = m_blueprintRegistry[m_blueprintCount++];
        bp.blueprintId = 5001;
        bp.targetItemId = 1002; // Powder Musket
        bp.requiredItemCount = 1;
        bp.requiredIronScrap = 25.0f; // Требует 25 единиц железа с заводов
        bp.requiredTechMods = 0.0f;
        bp.requiredToolId = 0;
    }

    // Рецепт 2: Восстановление стального торса Силовой Брони T-60 (Требует электронику)
    {
        BlueprintRecord& bp = m_blueprintRegistry[m_blueprintCount++];
        bp.blueprintId = 5002;
        bp.targetItemId = 2001; // T-60 Power Armor Torso
        bp.requiredItemCount = 1;
        bp.requiredIronScrap = 120.0f; // Тяжелый крафт
        bp.requiredTechMods = 15.0f;   // Требует модули Arknights фабрик
        bp.requiredToolId = 0;
    }

    // Рецепт 3: Сборка медицинского стимулятора Vault-Tec Stimpak
    {
        BlueprintRecord& bp = m_blueprintRegistry[m_blueprintCount++];
        bp.blueprintId = 5003;
        bp.targetItemId = 3001; // Stimpak
        bp.requiredItemCount = 2; // Создает сразу 2 штуки
        bp.requiredIronScrap = 5.0f;
        bp.requiredTechMods = 2.0f;
        bp.requiredToolId = 0;
    }

    Platform::Log("CraftingManager: Чертежи State of Decay 2 успешно загружены в реестр верстака.");
}

const BlueprintRecord* CraftingManager::GetBlueprint(uint32_t blueprintId) const noexcept {
    for (uint32_t i = 0; i < m_blueprintCount; ++i) {
        if (m_blueprintRegistry[i].blueprintId == blueprintId) {
            return &m_blueprintRegistry[i];
        }
    }
    return nullptr;
}

bool CraftingManager::TryExecuteCraft(uint32_t blueprintId, Player& player, FactorySystem& factoryContext, const ItemDatabase& itemDb) noexcept {
    const BlueprintRecord* bp = GetBlueprint(blueprintId);
    if (!bp) {
        Platform::Log("[CRAFTING ERROR]: Чертеж ID " + std::to_string(blueprintId) + " не найден.");
        return false;
    }

    // Использование itemDb передано через синглтон для архитектурного соответствия движка
    const auto* itemRecord = ItemDatabase::GetInstance().GetItemTemplatePtr(bp->targetItemId);
    if (!itemRecord) {
        Platform::Log("[CRAFTING ERROR]: Предмет не зарегистрирован в базе данных.");
        return false;
    }

    // Проверяем лимит бюджета фабрики (если лимит заполнен — крафтить нельзя)
    if (factoryContext.GetBudgetPercentage() > 95.0f) {
        Platform::Log("[CRAFTING FAIL]: Лимит бюджета строительства энергосети превышен!");
        return false;
    }

    player.AddItemToInventory(bp->targetItemId, bp->requiredItemCount);
    Platform::Log("[CRAFTING SUCCESS]: Собран предмет по чертежу " + std::to_string(blueprintId));
    return true;
}

} // namespace Centralia
