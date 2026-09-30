#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "platform/Platform.hpp"

namespace Centralia {

struct ShopOffer {
    uint32_t offerId;
    uint32_t itemId;
    uint32_t price;
    uint32_t stockCount;
    bool isAvailable;
};

class ServerShop {
private:
    std::unordered_map<uint32_t, ShopOffer> m_catalog;

    ServerShop() noexcept {
        InitializeDefaultCatalog();
    }

public:
    ~ServerShop() = default;

    ServerShop(const ServerShop&) = delete;
    ServerShop& operator=(const ServerShop&) = delete;

    static ServerShop& GetInstance() noexcept {
        static ServerShop instance;
        return instance;
    }

    void InitializeDefaultCatalog() noexcept;
    bool BuyItem(uint32_t offerId, uint32_t& playerCurrency) noexcept;
    bool SellItem(uint32_t itemId, uint32_t& playerCurrency) noexcept;

    [[nodiscard]] const ShopOffer* GetOffer(uint32_t offerId) const noexcept;
    [[nodiscard]] const std::unordered_map<uint32_t, ShopOffer>& GetCatalog() const noexcept { return m_catalog; }
};

} // namespace Centralia