#include "gameplay/ServerShop.hpp"
#include "platform/Platform.hpp"
#include "core/NetworkProtocol.hpp"

namespace Centralia {

void ServerShop::ConnectToSupportServer() {
    Platform::Log("[SHOP]: Попытка безопасного подключения к облаку поддержки хостингов...");
    
    // Эффект безопасного отключения: если коннект не прошел, мультиплеер не пострадает
    if (m_shopSocket.ConnectToServer(m_shopServerIp, m_shopPort)) {
        m_shopOnline = true;
        Platform::Log("[SHOP]: Синхронизация успешна. Доступен обмен ресурсами поддержки.");
    } else {
        m_shopOnline = false;
        Platform::Log("[SHOP] [ВНИМАНИЕ]: Мастер-сервер магазина недоступен. Режим доната отключен. Игровой P2P-кооператив работает в штатном режиме!");
    }
}

bool ServerShop::RequestResourcePurchase(uint32_t resourceId, uint32_t quantity) {
    if (!m_shopOnline) {
        Platform::Log("[SHOP ОШИБКА]: Функция недоступна в офлайн-режиме! Игра продолжается автономно.");
        return false;
    }

    // Собираем сетевой пакет покупки для отправки в сокет магазина
    NetworkPacket shopPacket;
    shopPacket.header.type = PacketType::InventorySync; // Используем базовый тип синхронизации предметов
    NetworkSerializer::WriteUInt32(shopPacket.payload, resourceId);
    NetworkSerializer::WriteUInt32(shopPacket.payload, quantity);

    std::vector<uint8_t> rawPacket = NetworkSerializer::Serialize(shopPacket);
    
    // Если во время отправки пакета интернет резко пропал — изолируем ошибку
    if (!m_shopSocket.SendBytes(rawPacket)) {
        m_shopOnline = false;
        Platform::Log("[SHOP] [ВНИМАНИЕ]: Связь с магазином оборвалась во время транзакции! Магазин безопасно изолирован. Локальная игра продолжается.");
        return false;
    }

    Platform::Log("[SHOP]: Пакет поддержки успешно обработан сервером.");
    return true;
}

void ServerShop::DisconnectShop() {
    m_shopSocket.Close();
    m_shopOnline = false;
}

} // namespace Centralia
