#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "platform/Platform.hpp"

namespace Centralia {

struct ProductionNode {
    uint32_t factoryId;
    uint32_t activeRecipeId;
    float productionProgress;
    bool isPowered;
};

class FactorySystem {
private:
    std::unordered_map<uint32_t, ProductionNode> m_factories;

    FactorySystem() noexcept;

public:
    ~FactorySystem() = default;

    FactorySystem(const FactorySystem&) = delete;
    FactorySystem& operator=(const FactorySystem&) = delete;

    static FactorySystem& GetInstance() noexcept {
        static FactorySystem instance;
        return instance;
    }

    void RegisterFactory(uint32_t factoryId);
    void StartProduction(uint32_t factoryId, uint32_t recipeId);
    void Update(float deltaTime);

    [[nodiscard]] bool GetFactoryStatus(uint32_t factoryId, ProductionNode& outNode) const noexcept;
};

} // namespace Centralia