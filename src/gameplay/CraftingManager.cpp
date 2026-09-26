#include "gameplay/CraftingManager.hpp"
#include "gameplay/FactorySystem.hpp"     // Этот инклуд обязан быть здесь для фикса C2027!
#include "gameplay/Player.hpp"
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

bool CraftingManager::TryExecuteCraft(uint32_t blueprintId, Player& player, FactoryEngineContext& factoryContext, const ItemDatabase& itemDb) noexcept {
    const BlueprintRecord* bp = GetBlueprint(blueprintId);
    if (!bp) {
        Platform::Log("[CRAFTING ERROR]: Чертеж ID " + std::to_string(blueprintId) + " не найден в реестре.");
        return false;
    }

    // Фикс опечатки: заменено blueprint->targetItemId на bp->targetItemId
    const auto* itemRecord = Centralia::ItemDatabase::GetInstance().GetItemTemplatePtr(bp->targetItemId);
    if (!itemRecord) {
        Platform::Log("[CRAFTING ERROR]: Целевой предмет крафта не зарегистрирован в ItemDatabase.");
        return false;
    }

    // Аппаратная проверка ресурсов, накопленных на хосте автоматическими фабриками
    if (factoryContext.GetIronScrap() < bp->requiredIronScrap) {
        Platform::Log("[CRAFTING FAIL]: Недостаточно металлолома. Требуется: " + std::to_string(bp->requiredIronScrap));
        return false;
    }

    // Симуляция списания ресурсов из бинарного слепка FactoryGridState
    factoryContext.DeductResources(bp->requiredIronScrap, bp->requiredTechMods);

    // Фикс аргументов: передаем ID предмета и количество напрямую без лишнего цикла
    player.AddItemToInventory(bp->targetItemId, bp->requiredItemCount);

    // Фикс поля: заменено itemRecord->itemName на твое родное поле itemRecord->name
    Platform::Log("[CRAFTING SUCCESS]: Верстак собрал '" + itemRecord->name + "' по чертежу " + std::to_string(blueprintId));
    return true;
}

} // namespace Centralia
