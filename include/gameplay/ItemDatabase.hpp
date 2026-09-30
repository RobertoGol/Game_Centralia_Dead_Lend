#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>
#include "platform/Platform.hpp"

namespace Centralia {

struct ItemDefinition {
    uint32_t itemId;
    std::string name;
    std::string description;
    uint32_t maxStackSize;
    float weight;
    bool isEquippable;
};

class ItemDatabase {
private:
    std::unordered_map<uint32_t, ItemDefinition> m_items;

    ItemDatabase() noexcept {
        InitializeDatabase();
    }

public:
    ~ItemDatabase() = default;

    ItemDatabase(const ItemDatabase&) = delete;
    ItemDatabase& operator=(const ItemDatabase&) = delete;

    static ItemDatabase& GetInstance() noexcept {
        static ItemDatabase instance;
        return instance;
    }

    void InitializeDatabase() noexcept;

    [[nodiscard]] bool GetItemDefinition(uint32_t itemId, ItemDefinition& outDef) const noexcept;
};

} // namespace Centralia