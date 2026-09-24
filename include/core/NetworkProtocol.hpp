#pragma once
#include <vector>
#include <string>
#include <cstdint>

namespace Centralia {

// Типы сетевых пакетов для синхронизации ПК и Android
enum class PacketType : uint16_t {
    Handshake     = 0x0100, // Первичное подключение и проверка HWID
    ChatMessage   = 0x0200, // Текстовое сообщение в чат
    PlayerState   = 0x0300, // Координаты и базовые статы игрока
    InventorySync = 0x0400  // Передача предметов при обыске ящиков/луте
};

// Фиксированный заголовок пакета (всего 8 байт)
struct PacketHeader {
    uint16_t magic_number = 0xCE44; // Магическое число "Centralia Dead Lend" (0xCE44)
    PacketType type;
    uint32_t payload_size;
};

// Сетевой пакет
struct NetworkPacket {
    PacketHeader header;
    std::vector<uint8_t> payload;
};

class NetworkSerializer {
public:
    // Побайтовая запись базовых типов данных в буфер (учитывает кроссплатформенность)
    static void WriteUInt16(std::vector<uint8_t>& buffer, uint16_t value);
    static void WriteUInt32(std::vector<uint8_t>& buffer, uint32_t value);
    static void WriteFloat(std::vector<uint8_t>& buffer, float value);
    static void WriteString(std::vector<uint8_t>& buffer, const std::string& value);

    // Побайтовое чтение из буфера пакета
    static uint16_t ReadUInt16(const std::vector<uint8_t>& buffer, size_t& offset);
    static uint32_t ReadUInt32(const std::vector<uint8_t>& buffer, size_t& offset);
    static float ReadFloat(const std::vector<uint8_t>& buffer, size_t& offset);
    static std::string ReadString(const std::vector<uint8_t>& buffer, size_t& offset);

    // Сборка готового сырого пакета для отправки в сеть
    static std::vector<uint8_t> Serialize(const NetworkPacket& packet);
    
    // Парсинг полученных из сети байтов в валидный пакет
    static bool Deserialize(const std::vector<uint8_t>& rawBytes, NetworkPacket& outPacket);
};

} // namespace Centralia
