#include "NetworkProtocol.hpp"
#include "platform/Platform.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <cmath>

namespace Centralia {

// ============================================================================
// SECTION 1: CONSTANTS, MAGIC NUMBERS & PROTOCOL CONFIGURATION
// ============================================================================

namespace ProtocolConfig {
    constexpr uint32_t PACKET_MAGIC = 0xC3NTR4L1; // "CENTRAL1"
    constexpr uint16_t PROTOCOL_VERSION = 1042;
    constexpr size_t MAX_UDP_PAYLOAD = 1400; // Безопасный MTU для UDP
    constexpr size_t MAX_PACKET_SIZE = 1024 * 64; // 64 KB макс. размер собранного пакета
}

// ============================================================================
// SECTION 2: BITSTREAM WRITER & READER (BANDWIDTH OPTIMIZATION)
// ============================================================================

class BitWriter {
private:
    std::vector<uint8_t>& m_buffer;
    uint32_t m_scratch;
    uint32_t m_scratchBits;

public:
    explicit BitWriter(std::vector<uint8_t>& buffer) : m_buffer(buffer), m_scratch(0), m_scratchBits(0) {}

    void WriteBits(uint32_t value, uint32_t bits) {
        if (bits == 0) return;
        value &= (1ULL << bits) - 1; // Маскируем лишние биты
        m_scratch |= (value << m_scratchBits);
        m_scratchBits += bits;

        while (m_scratchBits >= 8) {
            m_buffer.push_back(static_cast<uint8_t>(m_scratch & 0xFF));
            m_scratch >>= 8;
            m_scratchBits -= 8;
        }
    }

    void WriteFloat(float value) {
        uint32_t intVal;
        std::memcpy(&intVal, &value, sizeof(float));
        WriteBits(intVal, 32);
    }

    void WriteCompressedRotation(float angleDegrees) {
        // Упаковка угла 0-360 в 16 бит для экономии трафика (точность ~0.005 градуса)
        float normalized = std::fmod(angleDegrees, 360.0f);
        if (normalized < 0.0f) normalized += 360.0f;
        uint32_t packed = static_cast<uint32_t>((normalized / 360.0f) * 65535.0f);
        WriteBits(packed, 16);
    }

    void WriteString(const std::string& str) {
        uint32_t len = static_cast<uint32_t>(str.length());
        WriteBits(len, 16); // Максимум 65535 символов
        for (char c : str) {
            WriteBits(static_cast<uint32_t>(c), 8);
        }
    }

    void Flush() {
        if (m_scratchBits > 0) {
            m_buffer.push_back(static_cast<uint8_t>(m_scratch & 0xFF));
            m_scratch = 0;
            m_scratchBits = 0;
        }
    }
};

class BitReader {
private:
    const std::vector<uint8_t>& m_buffer;
    size_t m_byteIndex;
    uint32_t m_scratch;
    uint32_t m_scratchBits;

public:
    explicit BitReader(const std::vector<uint8_t>& buffer) 
        : m_buffer(buffer), m_byteIndex(0), m_scratch(0), m_scratchBits(0) {}

    uint32_t ReadBits(uint32_t bits) {
        if (bits == 0) return 0;

        while (m_scratchBits < bits) {
            if (m_byteIndex >= m_buffer.size()) {
                throw std::out_of_range("[PROTOCOL ERROR]: Попытка чтения за пределами буфера пакета!");
            }
            m_scratch |= (static_cast<uint32_t>(m_buffer[m_byteIndex++]) << m_scratchBits);
            m_scratchBits += 8;
        }

        uint32_t value = m_scratch & ((1ULL << bits) - 1);
        m_scratch >>= bits;
        m_scratchBits -= bits;
        return value;
    }

    float ReadFloat() {
        uint32_t intVal = ReadBits(32);
        float value;
        std::memcpy(&value, &intVal, sizeof(float));
        return value;
    }

    float ReadCompressedRotation() {
        uint32_t packed = ReadBits(16);
        return (static_cast<float>(packed) / 65535.0f) * 360.0f;
    }

    std::string ReadString() {
        uint32_t len = ReadBits(16);
        std::string str(len, '\0');
        for (uint32_t i = 0; i < len; ++i) {
            str[i] = static_cast<char>(ReadBits(8));
        }
        return str;
    }
    
    bool IsOverflow() const {
        return m_byteIndex >= m_buffer.size() && m_scratchBits == 0;
    }
};

// ============================================================================
// SECTION 3: ROBUST CRC32 IMPLEMENTATION (DATA INTEGRITY)
// ============================================================================

namespace CryptoTools {
    uint32_t CRC32Table[256];
    bool isCrcTableInitialized = false;

    void InitCRC32Table() {
        if (isCrcTableInitialized) return;
        uint32_t polynomial = 0xEDB88320;
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (size_t j = 0; j < 8; ++j) {
                if (c & 1) c = polynomial ^ (c >> 1);
                else c >>= 1;
            }
            CRC32Table[i] = c;
        }
        isCrcTableInitialized = true;
    }

    uint32_t ComputeCRC32(const uint8_t* data, size_t length) {
        InitCRC32Table();
        uint32_t c = 0xFFFFFFFF;
        for (size_t i = 0; i < length; ++i) {
            c = CRC32Table[(c ^ data[i]) & 0xFF] ^ (c >> 8);
        }
        return c ^ 0xFFFFFFFF;
    }
}

// ============================================================================
// SECTION 4: CHACHA20 STREAM CIPHER (PACKET ENCRYPTION)
// ============================================================================
// Написан с нуля. Шифрует payload пакета, чтобы читеры не могли читать/подменять
// координаты игроков и инвентарь (Packet Editing / Wallhacks).

namespace CryptoTools {
    inline uint32_t ROTL(uint32_t a, int b) {
        return (a << b) | (a >> (32 - b));
    }

    void ChaCha20QuarterRound(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d) {
        a += b; d ^= a; d = ROTL(d, 16);
        c += d; b ^= c; b = ROTL(b, 12);
        a += b; d ^= a; d = ROTL(d, 8);
        c += d; b ^= c; b = ROTL(b, 7);
    }

    void ChaCha20Block(uint32_t out[16], const uint32_t in[16]) {
        for (int i = 0; i < 16; ++i) out[i] = in[i];
        for (int i = 0; i < 10; ++i) {
            ChaCha20QuarterRound(out[0], out[4], out[8], out[12]);
            ChaCha20QuarterRound(out[1], out[5], out[9], out[13]);
            ChaCha20QuarterRound(out[2], out[6], out[10], out[14]);
            ChaCha20QuarterRound(out[3], out[7], out[11], out[15]);
            ChaCha20QuarterRound(out[0], out[5], out[10], out[15]);
            ChaCha20QuarterRound(out[1], out[6], out[11], out[12]);
            ChaCha20QuarterRound(out[2], out[7], out[8], out[13]);
            ChaCha20QuarterRound(out[3], out[4], out[9], out[14]);
        }
        for (int i = 0; i < 16; ++i) out[i] += in[i];
    }

    void ProcessChaCha20(const uint8_t* key, const uint8_t* nonce, uint32_t counter, uint8_t* data, size_t length) {
        uint32_t state[16];
        // Constants "expand 32-byte k"
        state[0] = 0x61707865; state[1] = 0x3320646e; state[2] = 0x79622d32; state[3] = 0x6b206574;
        
        std::memcpy(&state[4], key, 32);
        state[12] = counter;
        std::memcpy(&state[13], nonce, 12);

        uint32_t block[16];
        uint8_t blockBytes[64];
        
        size_t offset = 0;
        while (offset < length) {
            ChaCha20Block(block, state);
            std::memcpy(blockBytes, block, 64);
            
            size_t take = std::min(static_cast<size_t>(64), length - offset);
            for (size_t i = 0; i < take; ++i) {
                data[offset + i] ^= blockBytes[i];
            }
            offset += take;
            state[12]++; // Увеличиваем счетчик блоков
        }
    }
}

// ============================================================================
// SECTION 5: FRAGMENTATION & REASSEMBLY MANAGER
// ============================================================================

struct FragmentBuffer {
    uint32_t packetId;
    uint16_t totalFragments;
    uint16_t receivedFragments;
    std::vector<std::vector<uint8_t>> chunks;
    float timeoutTimer;
};

class FragmentManager {
private:
    std::vector<FragmentBuffer> m_activeBuffers;

public:
    void AddFragment(uint32_t packetId, uint16_t fragmentIdx, uint16_t totalFragments, const std::vector<uint8_t>& payload) {
        auto it = std::find_if(m_activeBuffers.begin(), m_activeBuffers.end(), 
                               [packetId](const FragmentBuffer& b) { return b.packetId == packetId; });
        
        if (it == m_activeBuffers.end()) {
            FragmentBuffer newBuffer;
            newBuffer.packetId = packetId;
            newBuffer.totalFragments = totalFragments;
            newBuffer.receivedFragments = 0;
            newBuffer.chunks.resize(totalFragments);
            newBuffer.timeoutTimer = 5.0f; // 5 секунд на сборку
            m_activeBuffers.push_back(newBuffer);
            it = m_activeBuffers.end() - 1;
        }

        if (it->chunks[fragmentIdx].empty()) {
            it->chunks[fragmentIdx] = payload;
            it->receivedFragments++;
        }
    }

    bool IsPacketReady(uint32_t packetId) const {
        auto it = std::find_if(m_activeBuffers.begin(), m_activeBuffers.end(), 
                               [packetId](const FragmentBuffer& b) { return b.packetId == packetId; });
        return (it != m_activeBuffers.end() && it->receivedFragments == it->totalFragments);
    }

    std::vector<uint8_t> AssemblePacket(uint32_t packetId) {
        std::vector<uint8_t> completePayload;
        auto it = std::find_if(m_activeBuffers.begin(), m_activeBuffers.end(), 
                               [packetId](const FragmentBuffer& b) { return b.packetId == packetId; });
        
        if (it != m_activeBuffers.end() && it->receivedFragments == it->totalFragments) {
            for (const auto& chunk : it->chunks) {
                completePayload.insert(completePayload.end(), chunk.begin(), chunk.end());
            }
            m_activeBuffers.erase(it); // Удаляем буфер после успешной сборки
        }
        return completePayload;
    }

    void TickTimeouts(float deltaTime) {
        for (auto it = m_activeBuffers.begin(); it != m_activeBuffers.end();) {
            it->timeoutTimer -= deltaTime;
            if (it->timeoutTimer <= 0.0f) {
                Platform::Log("[PROTOCOL WARNING]: Дропнут фрагментированный пакет ID " + std::to_string(it->packetId) + " (Timeout)");
                it = m_activeBuffers.erase(it);
            } else {
                ++it;
            }
        }
    }
};

// ============================================================================
// SECTION 6: NETWORK PROTOCOL MAIN INTERFACE
// ============================================================================

NetworkProtocol::NetworkProtocol() 
    : m_sequenceNumber(0),
      m_encryptionKey(32, 0),
      m_fragmentManager(new FragmentManager())
{
    // Генерация крипто-ключа (В реальности должен обмениваться через RSA/ECDH во время Handshake)
    const char* defaultKey = "C3ntr4l1a_N3tw0rk_K3y_256B1t_Sec";
    std::memcpy(m_encryptionKey.data(), defaultKey, 32);
    
    Platform::Log("[NETWORK PROTOCOL]: Сетевой протокол инициализирован. Включено шифрование ChaCha20.");
}

NetworkProtocol::~NetworkProtocol() {
    delete m_fragmentManager;
    Platform::Log("[NETWORK PROTOCOL]: Протокол выгружен из памяти.");
}

void NetworkProtocol::SetSessionEncryptionKey(const std::vector<uint8_t>& key32) {
    if (key32.size() == 32) {
        m_encryptionKey = key32;
        Platform::Log("[NETWORK PROTOCOL]: Ключ шифрования сессии успешно обновлен.");
    }
}

// ----------------------------------------------------------------------------
// СОЗДАНИЕ И УПАКОВКА ПАКЕТОВ (PACKET BUILDERS)
// ----------------------------------------------------------------------------

std::vector<std::vector<uint8_t>> NetworkProtocol::SerializePlayerTransform(uint32_t entityId, const Vector3D& pos, float yaw, float pitch, bool isCrouching, bool isSprinting) {
    std::vector<uint8_t> payload;
    BitWriter writer(payload);

    writer.WriteBits(static_cast<uint32_t>(PacketType::PlayerTransform), 8);
    writer.WriteBits(entityId, 32);
    
    writer.WriteFloat(pos.x);
    writer.WriteFloat(pos.y);
    writer.WriteFloat(pos.z);
    
    writer.WriteCompressedRotation(yaw);
    writer.WriteCompressedRotation(pitch);
    
    writer.WriteBits(isCrouching ? 1 : 0, 1);
    writer.WriteBits(isSprinting ? 1 : 0, 1);
    writer.Flush();

    return FinalizeAndFragmentPacket(payload, PacketType::PlayerTransform, true);
}

std::vector<std::vector<uint8_t>> NetworkProtocol::SerializePlayerState(uint32_t entityId, float health, float stamina, float radiation) {
    std::vector<uint8_t> payload;
    BitWriter writer(payload);

    writer.WriteBits(static_cast<uint32_t>(PacketType::PlayerState), 8);
    writer.WriteBits(entityId, 32);
    writer.WriteFloat(health);
    writer.WriteFloat(stamina);
    writer.WriteFloat(radiation);
    writer.Flush();

    // Состояния нужно отправлять гарантированно (Reliable)
    return FinalizeAndFragmentPacket(payload, PacketType::PlayerState, true);
}

std::vector<std::vector<uint8_t>> NetworkProtocol::SerializeWeaponFireEvent(uint32_t entityId, uint32_t weaponId, const Vector3D& origin, const Vector3D& direction) {
    std::vector<uint8_t> payload;
    BitWriter writer(payload);

    writer.WriteBits(static_cast<uint32_t>(PacketType::WeaponFire), 8);
    writer.WriteBits(entityId, 32);
    writer.WriteBits(weaponId, 32);
    
    writer.WriteFloat(origin.x);
    writer.WriteFloat(origin.y);
    writer.WriteFloat(origin.z);
    
    writer.WriteFloat(direction.x);
    writer.WriteFloat(direction.y);
    writer.WriteFloat(direction.z);
    writer.Flush();

    return FinalizeAndFragmentPacket(payload, PacketType::WeaponFire, true);
}

// ----------------------------------------------------------------------------
// ШИФРОВАНИЕ И ФРАГМЕНТАЦИЯ
// ----------------------------------------------------------------------------

std::vector<std::vector<uint8_t>> NetworkProtocol::FinalizeAndFragmentPacket(std::vector<uint8_t>& payload, PacketType type, bool isReliable) {
    m_sequenceNumber++;
    uint32_t packetId = m_sequenceNumber;

    // 1. Шифрование Payload
    uint8_t nonce[12] = {0}; // Nonce формируем из packetId
    std::memcpy(nonce, &packetId, sizeof(uint32_t));
    CryptoTools::ProcessChaCha20(m_encryptionKey.data(), nonce, 1, payload.data(), payload.size());

    // 2. Расчет фрагментов
    size_t chunkPayloadSize = ProtocolConfig::MAX_UDP_PAYLOAD - 24; // Оставляем 24 байта на Header
    uint16_t totalFragments = static_cast<uint16_t>(std::ceil(static_cast<float>(payload.size()) / chunkPayloadSize));
    
    std::vector<std::vector<uint8_t>> outPackets;

    for (uint16_t i = 0; i < totalFragments; ++i) {
        std::vector<uint8_t> packetData;
        packetData.reserve(ProtocolConfig::MAX_UDP_PAYLOAD);

        // Header
        uint32_t magic = ProtocolConfig::PACKET_MAGIC;
        packetData.insert(packetData.end(), reinterpret_cast<uint8_t*>(&magic), reinterpret_cast<uint8_t*>(&magic) + 4);
        
        uint16_t version = ProtocolConfig::PROTOCOL_VERSION;
        packetData.insert(packetData.end(), reinterpret_cast<uint8_t*>(&version), reinterpret_cast<uint8_t*>(&version) + 2);
        
        packetData.insert(packetData.end(), reinterpret_cast<uint8_t*>(&packetId), reinterpret_cast<uint8_t*>(&packetId) + 4);
        
        uint8_t flags = isReliable ? 0x01 : 0x00;
        packetData.push_back(flags);

        packetData.insert(packetData.end(), reinterpret_cast<uint8_t*>(&i), reinterpret_cast<uint8_t*>(&i) + 2); // Fragment Index
        packetData.insert(packetData.end(), reinterpret_cast<uint8_t*>(&totalFragments), reinterpret_cast<uint8_t*>(&totalFragments) + 2);

        // Payload Chunk
        size_t offset = i * chunkPayloadSize;
        size_t length = std::min(chunkPayloadSize, payload.size() - offset);
        packetData.insert(packetData.end(), payload.begin() + offset, payload.begin() + offset + length);

        // CRC32
        uint32_t crc = CryptoTools::ComputeCRC32(packetData.data(), packetData.size());
        packetData.insert(packetData.end(), reinterpret_cast<uint8_t*>(&crc), reinterpret_cast<uint8_t*>(&crc) + 4);

        outPackets.push_back(packetData);
    }

    return outPackets;
}

// ----------------------------------------------------------------------------
// ПАРСИНГ И ДЕСЕРИАЛИЗАЦИЯ ПАКЕТОВ
// ----------------------------------------------------------------------------

ParsedNetworkEvent NetworkProtocol::ParseIncomingPacket(const std::vector<uint8_t>& rawData) {
    ParsedNetworkEvent event;
    event.isValid = false;

    if (rawData.size() < 24) {
        Platform::Log("[PROTOCOL ERROR]: Размер пакета слишком мал для наличия заголовков.");
        return event;
    }

    // 1. Проверка CRC32 (последние 4 байта)
    uint32_t receivedCrc = 0;
    std::memcpy(&receivedCrc, &rawData[rawData.size() - 4], 4);
    uint32_t computedCrc = CryptoTools::ComputeCRC32(rawData.data(), rawData.size() - 4);
    
    if (receivedCrc != computedCrc) {
        Platform::Log("[PROTOCOL DROP]: Нарушена целостность пакета (CRC Mismatch).");
        return event;
    }

    size_t cursor = 0;

    // 2. Парсинг заголовка
    uint32_t magic;
    std::memcpy(&magic, rawData.data() + cursor, 4); cursor += 4;
    if (magic != ProtocolConfig::PACKET_MAGIC) {
        Platform::Log("[PROTOCOL DROP]: Неверный Magic Identifier.");
        return event;
    }

    uint16_t version;
    std::memcpy(&version, rawData.data() + cursor, 2); cursor += 2;
    if (version != ProtocolConfig::PROTOCOL_VERSION) {
        Platform::Log("[PROTOCOL DROP]: Конфликт версий протокола (" + std::to_string(version) + ").");
        return event;
    }

    uint32_t packetId;
    std::memcpy(&packetId, rawData.data() + cursor, 4); cursor += 4;
    
    uint8_t flags = rawData[cursor++];
    bool isReliable = (flags & 0x01) != 0;

    uint16_t fragmentIdx, totalFragments;
    std::memcpy(&fragmentIdx, rawData.data() + cursor, 2); cursor += 2;
    std::memcpy(&totalFragments, rawData.data() + cursor, 2); cursor += 2;

    // 3. Извлечение payload чанка
    size_t payloadSize = rawData.size() - cursor - 4; // минус CRC
    std::vector<uint8_t> chunkPayload(rawData.begin() + cursor, rawData.begin() + cursor + payloadSize);

    // 4. Сборка фрагментов
    if (totalFragments > 1) {
        m_fragmentManager->AddFragment(packetId, fragmentIdx, totalFragments, chunkPayload);
        if (!m_fragmentManager->IsPacketReady(packetId)) {
            // Ждем остальные фрагменты
            event.isValid = false; 
            return event;
        }
        chunkPayload = m_fragmentManager->AssemblePacket(packetId);
    }

    // 5. Дешифровка
    uint8_t nonce[12] = {0};
    std::memcpy(nonce, &packetId, sizeof(uint32_t));
    CryptoTools::ProcessChaCha20(m_encryptionKey.data(), nonce, 1, chunkPayload.data(), chunkPayload.size());

    // 6. Парсинг данных игрового события через BitReader
    try {
        BitReader reader(chunkPayload);
        event.type = static_cast<PacketType>(reader.ReadBits(8));

        switch (event.type) {
            case PacketType::PlayerTransform:
                event.entityId = reader.ReadBits(32);
                event.position.x = reader.ReadFloat();
                event.position.y = reader.ReadFloat();
                event.position.z = reader.ReadFloat();
                event.yaw = reader.ReadCompressedRotation();
                event.pitch = reader.ReadCompressedRotation();
                event.isCrouching = reader.ReadBits(1) != 0;
                event.isSprinting = reader.ReadBits(1) != 0;
                break;

            case PacketType::PlayerState:
                event.entityId = reader.ReadBits(32);
                event.health = reader.ReadFloat();
                event.stamina = reader.ReadFloat();
                event.radiation = reader.ReadFloat();
                break;

            case PacketType::WeaponFire:
                event.entityId = reader.ReadBits(32);
                event.weaponId = reader.ReadBits(32);
                event.position.x = reader.ReadFloat();
                event.position.y = reader.ReadFloat();
                event.position.z = reader.ReadFloat();
                event.forwardVector.x = reader.ReadFloat();
                event.forwardVector.y = reader.ReadFloat();
                event.forwardVector.z = reader.ReadFloat();
                break;
                
            default:
                Platform::Log("[PROTOCOL WARNING]: Получен неизвестный тип пакета.");
                return event;
        }

        event.isValid = true;
    } 
    catch (const std::exception& e) {
        Platform::Log(std::string("[PROTOCOL CORRUPTION]: Ошибка десериализации битового потока: ") + e.what());
    }

    return event;
}

void NetworkProtocol::UpdateTick(float deltaTime) {
    if (m_fragmentManager) {
        m_fragmentManager->TickTimeouts(deltaTime);
    }
}

} // namespace Centralia