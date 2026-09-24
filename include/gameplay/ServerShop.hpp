#pragma once
#include "core/NetworkSocket.hpp"
#include <string>

namespace Centralia {

class ServerShop {
private:
    NetworkSocket m_shopSocket;
    bool m_shopOnline = false;
    std::string m_shopServerIp = "35.289.97.966"; // IP из твоего системного терминала-референса
    uint16_t m_shopPort = 443;

    ServerShop() = default; // Сигнлтон

public:
    static ServerShop& GetInstance() {
        static ServerShop instance;
        return instance;
    }

    // Фоновая попытка зацепиться за сервер магазина
    void ConnectToSupportServer();

    // Проверка статуса: мультиплеер работает всегда, магазин — только если есть сеть
    bool IsShopOnline() const { return m_shopOnline; }

    // Покупка базовых ресурсов поддержки серверов хоста (Дерево, Железо, Энергия)
    bool RequestResourcePurchase(uint32_t resourceId, uint32_t quantity);
    
    void DisconnectShop();
};

} // namespace Centralia
