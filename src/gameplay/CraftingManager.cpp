#include "gameplay/CraftingManager.hpp"
#include "platform/Platform.hpp"
#include "gameplay/ItemDatabase.hpp"

namespace Centralia {

CraftingManager::CraftingManager() {}

void CraftingManager::Initialize() {
    m_recipes.clear();

    // Рецепт 1: Крафт Армейской аптечки (ID 301) из Грязной воды (ID 401) и кучи Металлолома для инструментов
    CraftingRecipe medkitRecipe;
    medkitRecipe.result_item_id = 301;
    medkitRecipe.result_quantity = 1;
    medkitRecipe.ingredients = { {401, 1}, {501, 2} }; // 1 грязная вода + 2 металлолома
    m_recipes.push_back(medkitRecipe);

    // Рецепт 2: Восстановление/Крафт Кожаной куртки (ID 201) из кусков Металлолома (ID 501)
    CraftingRecipe armorRecipe;
    armorRecipe.result_item_id = 201;
    armorRecipe.result_quantity = 1;
    armorRecipe.ingredients = { {501, 5} }; // 5 единиц металлолома
    m_recipes.push_back(armorRecipe);

    Platform::Log("CraftingManager: Workbench blueprints loaded (" + std::to_string(m_recipes.size()) + " recipes).");
}

int CraftingManager::FindItemIndex(const Player& player, uint32_t itemId) const {
    const auto& inv = player.GetInventory();
    for (size_t i = 0; i < inv.size(); ++i) {
        if (inv[i].id == itemId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool CraftingManager::CraftItem(Player& player, uint32_t recipeResultId) {
    // 1. Ищем нужный рецепт в базе верстака
    const CraftingRecipe* targetRecipe = nullptr;
    for (const auto& recipe : m_recipes) {
        if (recipe.result_item_id == recipeResultId) {
            targetRecipe = &recipe;
            break;
        }
    }

    if (!targetRecipe) {
        Platform::Log("Ошибка крафта: Чертеж не найден на этом верстаке.");
        return false;
    }

    // 2. Шаг проверки: хватает ли у игрока всех ресурсов в инвентаре?
    for (const auto& ing : targetRecipe->ingredients) {
        int idx = FindItemIndex(player, ing.item_id);
        if (idx == -1 || player.GetInventory()[idx].quantity < ing.quantity) {
            ItemTemplate missingItemData;
            ItemDatabase::GetInstance().GetTemplate(ing.item_id, missingItemData);
            Platform::Log("Недостаточно ресурсов для крафта! Требуется: " + missingItemData.name);
            return false; // Ресурсов не хватает, прерываем операцию
        }
    }

    // 3. Шаг удаления: раз всего хватает, изымаем хлам из инвентаря персонажа
    for (const auto& ing : targetRecipe->ingredients) {
        int idx = FindItemIndex(player, ing.item_id);
        player.RemoveItem(static_cast<size_t>(idx), ing.quantity);
    }

    // 4. Шаг выдачи: добавляем созданный чистый предмет в инвентарь
    player.AddItem(targetRecipe->result_item_id, targetRecipe->result_quantity, 1.0f); // 1.0f — новая прочность
    
    ItemTemplate resultData;
    ItemDatabase::GetInstance().GetTemplate(targetRecipe->result_item_id, resultData);
    Platform::Log(player.GetNickname() + " успешно создал на верстаке: " + resultData.name);
    
    return true;
}

} // namespace Centralia
