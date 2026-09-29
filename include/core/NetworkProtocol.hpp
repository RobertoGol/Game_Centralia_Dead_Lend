#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

namespace Centralia {

// Типы сетевых пакетов для синхронизации ПК и Android
enum class PacketType : uint16_t {
    Handshake     = 0x0100, // Первичное подключение и проверка HWID
    ChatMessage   = 0x0200, // Текстовое сообщение в чат
    PlayerState   = 0x0300, // Координаты и базовые статы игрока
    InventorySync = 0x0400  // Передача предметов при обыске ящиков/луте
};

// Фиксированный заголовок пакета (всего 8 байт)
#pragma pack(push, 1)
struct PacketHeader {
    uint16_t magic_number = 0xCE44; // Магическое число "Centralia Dead Lend" (0xCE44)
    PacketType type;
    uint32_t payload_size;
};
#pragma pack(pop)

// Сетевой пакет
struct NetworkPacket {
    PacketHeader header;
    std::vector<uint8_t> payload;
};

class NetworkSerializer {
public:
    // --- ЗАПИСЬ ДАННЫХ (Сериализация) ---

    inline static void WriteUInt16(std::vector<uint8_t>& buffer, uint16_t value)   {
        buffer.push_back(static_cast<uint8_t>(value & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    };

    inline static void WriteUInt32(std::vector<uint8_t>& buffer, uint32_t value)   {
        buffer.push_back(static_cast<uint8_t>(value & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    };

    inline static void WriteFloat(std::vector<uint8_t>& buffer, float value)   {
        uint32_t temp;
        std::memcpy(&temp, &value, sizeof(float));
        WriteUInt32(buffer, temp);
    };

    inline static void WriteString(std::vector<uint8_t>& buffer, const std::string& value)   {
        WriteUInt32(buffer, static_cast<uint32_t>(value.size()));
        buffer.insert(buffer.end(), value.begin(), value.end());
    };

    // --- ЧТЕНИЕ ДАННЫХ (Десериализация) ---

    inline static uint16_t ReadUInt16(const std::vector<uint8_t>& buffer, size_t& offset)   {
        if (offset + 2 > buffer.size()) return 0;
        uint16_t value = buffer[offset] | (buffer[offset + 1] << 8);
        offset += 2;
        return value;
    };

    inline static uint32_t ReadUInt32(const std::vector<uint8_t>& buffer, size_t& offset)   {
        if (offset + 4 > buffer.size()) return 0;
        uint32_t value = buffer[offset] | 
                         (buffer[offset + 1] << 8) | 
                         (buffer[offset + 2] << 16) | 
                         (buffer[offset + 3] << 24);
        offset += 4;
        return value;
    };

    inline static float ReadFloat(const std::vector<uint8_t>& buffer, size_t& offset)   {
        uint32_t temp = ReadUInt32(buffer, offset);
        float value;
        std::memcpy(&value, &temp, sizeof(float));
        return value;
    };

    inline static std::string ReadString(const std::vector<uint8_t>& buffer, size_t& offset)   {
        uint32_t length = ReadUInt32(buffer, offset);
        if (offset + length > buffer.size()) return "";
        
        std::string value(buffer.begin() + offset, buffer.begin() + offset + length);
        offset += length;
        return value;
    };

    // --- СБОРКА И ПАРСИНГ ПОЛНЫХ ПАКЕТОВ ---

    inline static std::vector<uint8_t> Serialize(const NetworkPacket& packet)   {
        std::vector<uint8_t> rawBytes;
        rawBytes.reserve(8 + packet.payload.size());

        WriteUInt16(rawBytes, packet.header.magic_number);
        WriteUInt16(rawBytes, static_cast<uint16_t>(packet.header.type));
        WriteUInt32(rawBytes, static_cast<uint32_t>(packet.payload.size()));

        rawBytes.insert(rawBytes.end(), packet.payload.begin(), packet.payload.end());
        return rawBytes;
    };
    
    // ИСПРАВЛЕНО: Внедрен ioOffset для безопасного пошагового вычитывания потока TCP-пакетов без потерь данных
    inline static bool Deserialize(const std::vector<uint8_t>& rawBytes, size_t& ioOffset, NetworkPacket& outPacket)   {
        if (ioOffset + 8 > rawBytes.size()) return false; 

        size_t localOffset = ioOffset;
        uint16_t magic = ReadUInt16(rawBytes, localOffset);
        
        if (magic != 0xCE44) return false; // Чужой пакет, сбрасываем чтение

        outPacket.header.magic_number = magic;
        outPacket.header.type = static_cast<PacketType>(ReadUInt16(rawBytes, localOffset));
        outPacket.header.payload_size = ReadUInt32(rawBytes, localOffset);

        if (localOffset + outPacket.header.payload_size > rawBytes.size()) return false; // Пакет пришел не целиком

        outPacket.payload.assign(rawBytes.begin() + localOffset, rawBytes.begin() + localOffset + outPacket.header.payload_size);
        
        localOffset += outPacket.header.payload_size;
        ioOffset = localOffset; // Фиксируем успешное прочтение пакета из общего стрима
        return true;
    };
};

}; // namespace Centralia
