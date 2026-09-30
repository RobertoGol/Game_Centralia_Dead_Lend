#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include "platform/Platform.hpp"
#include "core/Math3D.hpp"

namespace Centralia {

enum class PacketType : uint16_t {
    Handshake = 0x0001,
    LoginRequest = 0x0002,
    LoginResponse = 0x0003,
    PlayerMovement = 0x0010,
    PlayerAction = 0x0011,
    CraftRequest = 0x0020,
    CraftResponse = 0x0021,
    WorldStateSync = 0x0030,
    Disconnect = 0xFFFF
};

#pragma pack(push, 1)
struct PacketHeader {
    uint16_t magicNumber = 0xC3E7; // Сигнатура движка Centralia
    PacketType type;
    uint32_t payloadSize;
};
#pragma pack(pop)

class NetworkProtocol {
private:
    bool m_isCompressed;

public:
    NetworkProtocol() noexcept;
    ~NetworkProtocol() = default;

    NetworkProtocol(const NetworkProtocol&) = delete;
    NetworkProtocol& operator=(const NetworkProtocol&) = delete;

    std::vector<uint8_t> SerializePacket(PacketType type, const std::vector<uint8_t>& payload) const;
    bool DeserializePacket(const std::vector<uint8_t>& rawData, PacketHeader& outHeader, std::vector<uint8_t>& outPayload) const;

    [[nodiscard]] bool IsCompressionEnabled() const noexcept { return m_isCompressed; }
    void SetCompressionEnabled(bool enabled) noexcept { m_isCompressed = enabled; }
};

} // namespace Centralia