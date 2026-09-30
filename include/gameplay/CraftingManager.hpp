#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "platform/Platform.hpp"

namespace Centralia {

struct CraftingRecipe {
    uint32_t recipeId;
    std::string resultItemName;
    uint32_t outputQuantity;
    std::unordered_map<uint32_t, uint32_t> requiredIngredients; // itemId -> quantity
    float craftingTimeSeconds;
};

class CraftingManager {
private:
    std::unordered_map<uint32_t, CraftingRecipe> m_recipes;

    CraftingManager() noexcept {
        RegisterDefaultRecipes();
    }

public:
    ~CraftingManager() = default;

    CraftingManager(const CraftingManager&) = delete;
    CraftingManager& operator=(const CraftingManager&) = delete;

    static CraftingManager& GetInstance() noexcept {
        static CraftingManager instance;
        return instance;
    }

    void RegisterDefaultRecipes() noexcept;
    bool CanCraft(uint32_t recipeId, const std::unordered_map<uint32_t, uint32_t>& playerInventory) const noexcept;
    bool CraftItem(uint32_t recipeId, std::unordered_map<uint32_t, uint32_t>& playerInventory) noexcept;

    [[nodiscard]] const CraftingRecipe* GetRecipe(uint32_t recipeId) const noexcept;
};

} // namespace Centralia