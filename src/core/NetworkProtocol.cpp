#include "core/NetworkSerializer.hpp" // ИСПРАВЛЕНО: Подключаем каноничный хедер вместо NetworkProtocol.hpp
#include <cstring>
#include <algorithm>

namespace Centralia {

// --- ЗАПИСЬ ДАННЫХ (Сериализация) ---

void NetworkSerializer::WriteUInt16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void NetworkSerializer::WriteUInt32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void NetworkSerializer::WriteFloat(std::vector<uint8_t>& buffer, float value) {
    uint32_t temp;
    std::memcpy(&temp, &value, sizeof(float)); 
    WriteUInt32(buffer, temp);
}

void NetworkSerializer::WriteString(std::vector<uint8_t>& buffer, const std::string& value) {
    WriteUInt32(buffer, static_cast<uint32_t>(value.size()));
    buffer.insert(buffer.end(), value.begin(), value.end());
}

// --- ЧТЕНИЕ ДАННЫХ (Десериализация) ---

uint16_t NetworkSerializer::ReadUInt16(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (offset + 2 > buffer.size()) return 0;
    uint16_t value = buffer[offset] | (buffer[offset + 1] << 8);
    offset += 2;
    return value;
}

uint32_t NetworkSerializer::ReadUInt32(const std::vector<uint8_t>& buffer, size_t& offset) {
    if (offset + 4 > buffer.size()) return 0;
    uint32_t value = buffer[offset] | 
                     (buffer[offset + 1] << 8) | 
                     (buffer[offset + 2] << 16) | 
                     (buffer[offset + 3] << 24);
    offset += 4;
    return value;
}

float NetworkSerializer::ReadFloat(const std::vector<uint8_t>& buffer, size_t& offset) {
    uint32_t temp = ReadUInt32(buffer, offset);
    float value;
    std::memcpy(&value, &temp, sizeof(float));
    return value;
}

std::string NetworkSerializer::ReadString(const std::vector<uint8_t>& buffer, size_t& offset) {
    uint32_t length = ReadUInt32(buffer, offset);
    if (offset + length > buffer.size()) return "";
    
    std::string value(buffer.begin() + offset, buffer.begin() + offset + length);
    offset += length;
    return value;
}

// --- СБОРКА И ПАРСИНГ ПОЛНЫХ ПАКЕТОВ ---

std::vector<uint8_t> NetworkSerializer::Serialize(const NetworkPacket& packet) {
    std::vector<uint8_t> rawBytes;
    rawBytes.reserve(8 + packet.payload.size());

    WriteUInt16(rawBytes, packet.header.magic_number);
    WriteUInt16(rawBytes, static_cast<uint16_t>(packet.header.type));
    WriteUInt32(rawBytes, static_cast<uint32_t>(packet.payload.size()));

    rawBytes.insert(rawBytes.end(), packet.payload.begin(), packet.payload.end());
    return rawBytes;
}

// ИСПРАВЛЕНО: Интегрирован ioOffset. Метод больше не стирает склеенные TCP-пакеты из буфера сети.
bool NetworkSerializer::Deserialize(const std::vector<uint8_t>& rawBytes, size_t& ioOffset, NetworkPacket& outPacket) {
    if (ioOffset + 8 > rawBytes.size()) return false; 

    size_t localOffset = ioOffset;
    uint16_t magic = ReadUInt16(rawBytes, localOffset);
    
    if (magic != 0xCE44) return false; // Защита от мусорных пакетов

    outPacket.header.magic_number = magic;
    outPacket.header.type = static_cast<PacketType>(ReadUInt16(rawBytes, localOffset));
    outPacket.header.payload_size = ReadUInt32(rawBytes, localOffset);

    if (localOffset + outPacket.header.payload_size > rawBytes.size()) return false; 

    outPacket.payload.assign(rawBytes.begin() + localOffset, rawBytes.begin() + localOffset + outPacket.header.payload_size);
    
    localOffset += outPacket.header.payload_size;
    ioOffset = localOffset; // Сдвигаем глобальный указатель чтения для обработки следующего пакета в цикле
    return true;
}

} // namespace Centralia
