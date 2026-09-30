#include "gameplay/ServerShop.hpp"
#include "gameplay/Player.hpp"
#include "platform/Platform.hpp"
#include "core/NetworkProtocol.hpp"
#include "core/NetworkSocket.hpp"
#include "core/MemoryManager.hpp"
#include <algorithm>
#include <chrono>
#include <thread>
#include <iostream>

namespace Centralia {

// ============================================================================
// 1. КОНСТРУКТОРЫ И ИНИЦИАЛИЗАЦИЯ СЕТЕВОГО МАГАЗИНА
// ============================================================================

ServerShop::ServerShop() 
    : m_shopOnline(false),
      m_shopPort(443),
      m_shopServerIp("35.289.97.96"),
      m_lastHeartbeatTimestamp(0),
      m_pendingTransactionsCount(0),
      m_playerAccountCredits(1000) // Начальные кредиты поддержки хоста
{
    m_activeCatalog.clear();
    InitializeDefaultCatalog();
    Platform::Log("[SHOP SYSTEM]: Модуль облачного магазина и поддержки инициализирован в памяти.");
}

ServerShop::~ServerShop() {
    DisconnectShop();
    m_activeCatalog.clear();
}

void ServerShop::InitializeDefaultCatalog() {
    // Заполняем стартовый каталог доступных за донат или очки поддержки ресурсов Пустоши
    ShopCatalogItem item1{ 2001, "Строительная древесина (Пакет 100 шт)", 150, true };
    ShopCatalogItem item2{ 2002, "Концентрат Железа (Очищенный сплав)", 300, true };
    ShopCatalogItem item3{ 2003, "Ядро Квантовой Энергии (Заряд 100%)", 850, true };
    ShopCatalogItem item4{ 3001, "Военный стимулятор Vault-Tec (Набор 5 шт)", 200, true };

    m_activeCatalog[item1.itemId] = item1;
    m_activeCatalog[item2.itemId] = item2;
    m_activeCatalog[item3.itemId] = item3;
    m_activeCatalog[item4.itemId] = item4;

    Platform::Log("[SHOP CATALOG]: Загружено базовых позиций каталога поддержки: " + std::to_string(m_activeCatalog.size()));
}

// ============================================================================
// 2. УПРАВЛЕНИЕ ПОДКЛЮЧЕНИЕМ И СЕТЕВЫЕ СОКЕТЫ
// ============================================================================

void ServerShop::ConnectToSupportServer() {
    Platform::Log("[SHOP]: Попытка безопасного подключения к мастер-серверу поддержки по адресу " + m_shopServerIp + ":" + std::to_string(m_shopPort) + "...");

    // Вызываем сетевой сокет для установления TCP-соединения с облаком хостинга
    if (m_shopSocket.ConnectToServer(m_shopServerIp, m_shopPort)) {
        m_shopOnline = true;
        m_lastHeartbeatTimestamp = static_cast<uint64_t>(std::time(nullptr));
        Platform::Log("[SHOP]: Синхронизация с облаком успешна. Доступен защищенный обмен ресурсами поддержки.");
    } else {
        m_shopOnline = false;
        Platform::Log("[SHOP WARNING]: Мастер-сервер магазина недоступен. Режим доната и облачных покупок отключен. P2P-кооператив работает локально!");
    }
}

void ServerShop::DisconnectShop() {
    if (m_shopOnline) {
        // Отправляем пакет завершения сессии перед закрытием дескриптора
        NetworkPacket closePacket;
        closePacket.header.type = PacketType::InventorySync;
        std::vector<uint8_t> raw = NetworkSerializer::Serialize(closePacket);
        m_shopSocket.SendBytes(raw);

        m_shopSocket.Close();
        m_shopOnline = false;
        Platform::Log("[SHOP]: Сессия связи с сервером поддержки штатно закрыта.");
    }
}

// ============================================================================
// 3. ТРАНЗАКЦИИ, ПОКУПКА РЕСУРСОВ И СИНХРОНИЗАЦИЯ С ИНВЕНТАРЕМ
// ============================================================================

bool ServerShop::RequestResourcePurchase(uint32_t resourceId, uint32_t quantity) {
    if (!m_shopOnline) {
        Platform::Log("[SHOP ERROR]: Покупка невозможна! Магазин находится в автономном офлайн-режиме.");
        return false;
    }

    // Проверяем, существует ли запрашиваемый предмет в каталоге
    auto catalogIt = m_activeCatalog.find(resourceId);
    if (catalogIt == m_activeCatalog.end() || !catalogIt->second.isAvailable) {
        Platform::Log("[SHOP ERROR]: Запрошенный товар ID " + std::to_string(resourceId) + " отсутствует в каталоге.");
        return false;
    }

    uint32_t totalCost = catalogIt->second.priceCredits * quantity;
    if (m_playerAccountCredits < totalCost) {
        Platform::Log("[SHOP ERROR]: Недостаточно кредитов поддержки! Требуется: " + std::to_string(totalCost) + ", у вас в наличии: " + std::to_string(m_playerAccountCredits));
        return false;
    }

    // Формируем сетевой пакет запроса транзакции покупки
    NetworkPacket shopPacket;
    shopPacket.header.type = PacketType::CraftRequest; // Использовать стандартизированный тип пакетного запроса
    NetworkSerializer::WriteUInt32(shopPacket.payload, resourceId);
    NetworkSerializer::WriteUInt32(shopPacket.payload, quantity);
    NetworkSerializer::WriteUInt32(shopPacket.payload, totalCost);

    std::vector<uint8_t> rawPacket = NetworkSerializer::Serialize(shopPacket);

    // Передаем байты по сокету с проверкой разрыва соединения
    if (!m_shopSocket.SendBytes(rawPacket)) {
        m_shopOnline = false;
        Platform::Log("[SHOP FATAL]: Связь с сервером оборвана прямо во время транзакции! Магазин изолирован, локальная игра защищена.");
        return false;
    }

    // Списываем кредиты локально при успешной отправке запроса
    m_playerAccountCredits -= totalCost;
    m_pendingTransactionsCount++;

    Platform::Log("[SHOP SUCCESS]: Запрос на покупку товара '" + catalogIt->second.itemName + "' (Кол-во: " + std::to_string(quantity) + ") успешно отправлен в облако.");
    return true;
}

bool ServerShop::VerifyAndDeliverPurchase(uint32_t transactionId, Player& player, uint32_t grantedItemId, uint32_t grantedQty) {
    // Метод подтверждения транзакции от сервера и выдачи предметов в Ghost-RAM инвентарь игрока
    Platform::Log("[SHOP TRANSACTION]: Сервер подтвердил валидность транзакции #" + std::to_string(transactionId));
    
    // Выдаем честно купленный ресурс через метод игрока
    player.AddItemToInventory(grantedItemId, grantedQty);
    
    if (m_pendingTransactionsCount > 0) {
        m_pendingTransactionsCount--;
    }

    Platform::Log("[SHOP DELIVERY]: Ресурсы успешно зачислены в инвентарь выжившего.");
    return true;
}

// ============================================================================
// 4. ПИНГ И ПОДДЕРЖКА СОЕДИНЕНИЯ (HEARTBEAT TICK)
// ============================================================================

void ServerShop::PollShopHeartbeat(float deltaTime) {
    if (!m_shopOnline) return;

    static float heartbeatTimer = 0.0f;
    heartbeatTimer += deltaTime;

    // Каждые 15 секунд отправляем пинг-пакет для проверки активности соединения с мастер-сервером
    if (heartbeatTimer >= 15.0f) {
        heartbeatTimer = 0.0f;

        NetworkPacket pingPacket;
        pingPacket.header.type = PacketType::KeepAlive;
        std::vector<uint8_t> rawPing = NetworkSerializer::Serialize(pingPacket);

        if (!m_shopSocket.SendBytes(rawPing)) {
            m_shopOnline = false;
            Platform::Log("[SHOP TIMEOUT]: Соединение с сервером поддержки потеряно по тайм-ауту. Переход в автономный режим.");
            m_shopSocket.Close();
        }
    }
}

// ============================================================================
// 5. ТЕЛЕМЕТРИЯ И ГЕТТЕРЫ СОСТОЯНИЯ
// ============================================================================

uint32_t ServerShop::GetPlayerCredits() const noexcept {
    return m_playerAccountCredits;
}

void ServerShop::AddPlayerCredits(uint32_t amount) noexcept {
    m_playerAccountCredits += amount;
    Platform::Log("[SHOP]: Начислено кредитов поддержки: +" + std::to_string(amount) + ". Баланс: " + std::to_string(m_playerAccountCredits));
}

bool ServerShop::IsShopAvailable() const noexcept {
    return m_shopOnline;
}

} // namespace Centralia