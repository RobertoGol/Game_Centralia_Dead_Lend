#pragma once
#include "gameplay/Player.hpp"
#include <vector>

namespace Centralia {

// Структура необходимого ингредиента для рецепта
struct Ingredient {
    uint32_t item_id;
    uint16_t quantity;
};

// Структура самого рецепта крафта
struct CraftingRecipe {
    uint32_t result_item_id;       // Что получим на выходе
    uint16_t result_quantity;      // Сколько штук получим
    std::vector<Ingredient> ingredients; // Из чего крафтим
};

class CraftingManager {
private:
    std::vector<CraftingRecipe> m_recipes;

    CraftingManager(); // Синглтон

    // Поиск индекса предмета в инвентаре для удобства проверок
    int FindItemIndex(const Player& player, uint32_t itemId) const;

public:
    static CraftingManager& GetInstance() {
        static CraftingManager instance;
        return instance;
    }

    // Загрузка базовых рецептов (патроны, аптечки, ремонт)
    void Initialize();

    // Главный метод: попытка скрафтить предмет игроком
    bool CraftItem(Player& player, uint32_t recipeResultId);
    
    const std::vector<CraftingRecipe>& GetRecipes() const { return m_recipes; }
};

} // namespace Centralia
