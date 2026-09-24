#include "core/NetworkProtocol.hpp"
#include <cstring>

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
    std::memcpy(&temp, &value, sizeof(float)); // Безопасное копирование float в побайтовое представление
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

    // Записываем заголовок пакета
    WriteUInt16(rawBytes, packet.header.magic_number);
    WriteUInt16(rawBytes, static_cast<uint16_t>(packet.header.type));
    WriteUInt32(rawBytes, static_cast<uint32_t>(packet.payload.size()));

    // Добавляем полезную нагрузку (данные игрока или чата)
    rawBytes.insert(rawBytes.end(), packet.payload.begin(), packet.payload.end());
    return rawBytes;
}

bool NetworkSerializer::Deserialize(const std::vector<uint8_t>& rawBytes, NetworkPacket& outPacket) {
    if (rawBytes.size() < 8) return false; // Пакет слишком мал (битый заголовок)

    size_t offset = 0;
    outPacket.header.magic_number = ReadUInt16(rawBytes, offset);
    
    // Проверка сигнатуры: если это не наш пакет, сразу его отбрасываем
    if (outPacket.header.magic_number != 0xCE44) return false;

    outPacket.header.type = static_cast<PacketType>(ReadUInt16(rawBytes, offset));
    outPacket.header.payload_size = ReadUInt32(rawBytes, offset);

    if (offset + outPacket.header.payload_size > rawBytes.size()) return false; // Данные не полные

    outPacket.payload.assign(rawBytes.begin() + offset, rawBytes.begin() + offset + outPacket.header.payload_size);
    return true;
}

} // namespace Centralia
