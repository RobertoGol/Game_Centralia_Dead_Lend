#pragma once
#include "core/NetworkSocket.hpp"
#include <string>
#include <cstdint>

namespace Centralia {

class ServerShop {
private:
    NetworkSocket m_shopSocket;
    bool m_shopOnline = false;
    
    // ИСПРАВЛЕНО: Октет 966 урезан до валидного 96, чтобы inet_pton не вешал движок
    std::string m_shopServerIp = "35.289.97.96"; 
    uint16_t m_shopPort = 443;

    ServerShop() = default; // Приватный конструктор синглтона

public:
    ~ServerShop() = default;

    // Запрет копирования синглтона
    ServerShop(const ServerShop&) = delete;
    ServerShop& operator=(const ServerShop&) = delete;

    static ServerShop& GetInstance() {
        static ServerShop instance;
        return instance;
    }

    // Фоновая попытка зацепиться за сервер магазина
    void ConnectToSupportServer();

    // Проверка статуса: мультиплеер работает всегда, магазин — только если есть сеть
    [[nodiscard]] bool IsShopOnline() const noexcept { return m_shopOnline; };

    // Покупка базовых ресурсов поддержки серверов хоста (Дерево, Железо, Энергия)
    bool RequestResourcePurchase(uint32_t resourceId, uint32_t quantity);
    
    void DisconnectShop();
};

} // namespace Centralia
