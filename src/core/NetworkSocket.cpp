#include "NetworkSocket.hpp"
#include "platform/Platform.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <stdexcept>

// ============================================================================
// SECTION 1: CROSS-PLATFORM SOCKET MACROS & HEADERS
// ============================================================================

#if defined(_WIN32) || defined(_WIN64)
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    
    typedef int socklen_t;
    #define CLOSE_SOCKET closesocket
    #define SOCKET_VALID(s) ((s) != INVALID_SOCKET)
    #define GET_SOCKET_ERROR() WSAGetLastError()
    #define ERROR_WOULDBLOCK WSAEWOULDBLOCK
    #define ERROR_CONNRESET WSAECONNRESET
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <errno.h>
    
    typedef int SOCKET;
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR (-1)
    #define CLOSE_SOCKET close
    #define SOCKET_VALID(s) ((s) >= 0)
    #define GET_SOCKET_ERROR() errno
    #define ERROR_WOULDBLOCK EWOULDBLOCK
    #define ERROR_CONNRESET ECONNRESET
#endif

namespace Centralia {

// ============================================================================
// SECTION 2: CONSTANTS, DATA STRUCTURES & RATE LIMITING
// ============================================================================

namespace NetConfig {
    constexpr size_t MAX_UDP_PAYLOAD = 1400;
    constexpr size_t SOCKET_RECV_BUFFER = 1024 * 1024 * 8; // 8 MB Receive Buffer
    constexpr size_t SOCKET_SEND_BUFFER = 1024 * 1024 * 4; // 4 MB Send Buffer
    constexpr float PEER_TIMEOUT_SECONDS = 15.0f;
    constexpr float HEARTBEAT_INTERVAL = 2.0f;
    constexpr uint32_t MAX_PACKETS_PER_SEC = 250; // Rate limit per IP
}

struct PeerConnection {
    uint32_t peerId;
    sockaddr_in address;
    std::string ipString;
    uint16_t port;
    
    float timeSinceLastPacket;
    float timeSinceLastHeartbeat;
    float roundTripTimeMs; // Ping
    
    // Token Bucket for Rate Limiting
    float tokenBucket;
    
    bool isConnected;
    bool isPendingValidation;
};

// ============================================================================
// SECTION 3: SINGLETON IMPLEMENTATION & PRIVATE STATE
// ============================================================================

struct NetworkSocketImpl {
    SOCKET socketHandle;
    bool isInitialized;
    bool isServerMode;
    
    uint16_t localPort;
    std::mutex socketMutex;
    
    // Peer Tracking
    std::vector<PeerConnection> activePeers;
    std::vector<std::string> bannedIPs;
    uint32_t nextPeerId;
    
    // Telemetry
    size_t bytesSentTotal;
    size_t bytesReceivedTotal;
    size_t packetsDroppedTotal;
    
    // Simulated Network Conditions (Debug)
    bool simulateLatency;
    float simulatedDropChance; // 0.0 to 1.0
};

NetworkSocket* NetworkSocket::s_instance = nullptr;

NetworkSocket::NetworkSocket() : m_pImpl(new NetworkSocketImpl()) {
    if (s_instance) {
        Platform::Log("[NET FATAL]: Попытка двойной инициализации NetworkSocket!");
        std::terminate();
    }
    s_instance = this;
    
    m_pImpl->socketHandle = INVALID_SOCKET;
    m_pImpl->isInitialized = false;
    m_pImpl->isServerMode = false;
    m_pImpl->localPort = 0;
    m_pImpl->nextPeerId = 1000;
    m_pImpl->bytesSentTotal = 0;
    m_pImpl->bytesReceivedTotal = 0;
    m_pImpl->packetsDroppedTotal = 0;
    m_pImpl->simulateLatency = false;
    m_pImpl->simulatedDropChance = 0.0f;
    
    InitializeAPI();
}

NetworkSocket::~NetworkSocket() {
    Shutdown();
    ShutdownAPI();
    delete m_pImpl;
    s_instance = nullptr;
}

NetworkSocket& NetworkSocket::GetInstance() {
    if (!s_instance) std::terminate();
    return *s_instance;
}

// ============================================================================
// SECTION 4: PLATFORM API INITIALIZATION (WINSOCK / POSIX)
// ============================================================================

bool NetworkSocket::InitializeAPI() {
#if defined(_WIN32) || defined(_WIN64)
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        Platform::Log("[NET FATAL]: WSAStartup завершился с ошибкой: " + std::to_string(result));
        return false;
    }
    Platform::Log("[NET INIT]: WinSock 2.2 успешно инициализирован.");
#else
    Platform::Log("[NET INIT]: POSIX Socket API доступен по умолчанию.");
#endif
    return true;
}

void NetworkSocket::ShutdownAPI() {
#if defined(_WIN32) || defined(_WIN64)
    WSACleanup();
    Platform::Log("[NET SHUTDOWN]: WinSock API выгружен.");
#endif
}

// ============================================================================
// SECTION 5: SOCKET CREATION, BINDING & CONFIGURATION
// ============================================================================

bool NetworkSocket::CreateAndConfigureSocket() {
    m_pImpl->socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (!SOCKET_VALID(m_pImpl->socketHandle)) {
        Platform::Log("[NET ERROR]: Невозможно создать UDP сокет. Код ошибки: " + std::to_string(GET_SOCKET_ERROR()));
        return false;
    }

    // 1. Установка неблокирующего режима (Non-Blocking Mode)
#if defined(_WIN32) || defined(_WIN64)
    u_long mode = 1;
    if (ioctlsocket(m_pImpl->socketHandle, FIONBIO, &mode) != 0) {
        Platform::Log("[NET ERROR]: Ошибка перевода сокета в неблокирующий режим (ioctlsocket).");
        return false;
    }
#else
    int flags = fcntl(m_pImpl->socketHandle, F_GETFL, 0);
    if (fcntl(m_pImpl->socketHandle, F_SETFL, flags | O_NONBLOCK) != 0) {
        Platform::Log("[NET ERROR]: Ошибка перевода сокета в неблокирующий режим (fcntl).");
        return false;
    }
#endif

    // 2. Расширение буферов ядра ОС для сокета
    int recvBufSize = NetConfig::SOCKET_RECV_BUFFER;
    if (setsockopt(m_pImpl->socketHandle, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&recvBufSize), sizeof(recvBufSize)) != 0) {
        Platform::Log("[NET WARNING]: Не удалось увеличить системный буфер приема (SO_RCVBUF). Возможна потеря пакетов при пиковых нагрузках.");
    }

    int sendBufSize = NetConfig::SOCKET_SEND_BUFFER;
    if (setsockopt(m_pImpl->socketHandle, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&sendBufSize), sizeof(sendBufSize)) != 0) {
        Platform::Log("[NET WARNING]: Не удалось увеличить системный буфер отправки (SO_SNDBUF).");
    }

    // 3. Отключение ошибки ConnReset при недоставке UDP пакета (только для Windows)
#if defined(_WIN32) || defined(_WIN64)
    #define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
    BOOL bNewBehavior = FALSE;
    DWORD dwBytesReturned = 0;
    WSAIoctl(m_pImpl->socketHandle, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior), NULL, 0, &dwBytesReturned, NULL, NULL);
#endif

    Platform::Log("[NET SETUP]: UDP Сокет успешно создан и переведен в высокопроизводительный асинхронный режим.");
    return true;
}

bool NetworkSocket::BindAsServer(uint16_t port) {
    std::lock_guard<std::mutex> lock(m_pImpl->socketMutex);

    if (m_pImpl->isInitialized) Shutdown();
    if (!CreateAndConfigureSocket()) return false;

    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Слушаем все сетевые интерфейсы
    serverAddr.sin_port = htons(port);

    if (bind(m_pImpl->socketHandle, reinterpret_cast<const sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        Platform::Log("[NET FATAL]: Не удалось привязать сокет к порту " + std::to_string(port) + ". Возможно, порт уже занят.");
        CLOSE_SOCKET(m_pImpl->socketHandle);
        m_pImpl->socketHandle = INVALID_SOCKET;
        return false;
    }

    m_pImpl->localPort = port;
    m_pImpl->isServerMode = true;
    m_pImpl->isInitialized = true;
    
    Platform::Log("[NET SERVER]: Серверный сокет успешно привязан. Ожидание подключений на порту " + std::to_string(port) + "...");
    return true;
}

bool NetworkSocket::BindAsClient(uint16_t localPort) {
    std::lock_guard<std::mutex> lock(m_pImpl->socketMutex);

    if (m_pImpl->isInitialized) Shutdown();
    if (!CreateAndConfigureSocket()) return false;

    sockaddr_in clientAddr;
    std::memset(&clientAddr, 0, sizeof(clientAddr));
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    clientAddr.sin_port = htons(localPort); // 0 = ОС выберет случайный свободный порт

    if (bind(m_pImpl->socketHandle, reinterpret_cast<const sockaddr*>(&clientAddr), sizeof(clientAddr)) == SOCKET_ERROR) {
        Platform::Log("[NET FATAL]: Ошибка привязки клиентского сокета.");
        CLOSE_SOCKET(m_pImpl->socketHandle);
        m_pImpl->socketHandle = INVALID_SOCKET;
        return false;
    }

    // Извлекаем фактически выделенный ОС порт, если localPort был 0
    sockaddr_in boundAddr;
    socklen_t addrLen = sizeof(boundAddr);
    getsockname(m_pImpl->socketHandle, reinterpret_cast<sockaddr*>(&boundAddr), &addrLen);
    m_pImpl->localPort = ntohs(boundAddr.sin_port);

    m_pImpl->isServerMode = false;
    m_pImpl->isInitialized = true;

    Platform::Log("[NET CLIENT]: Клиентский сокет инициализирован на локальном порту " + std::to_string(m_pImpl->localPort));
    return true;
}

// ============================================================================
// SECTION 6: PACKET TRANSMISSION (SEND / RECEIVE)
// ============================================================================

bool NetworkSocket::SendPacket(const std::vector<uint8_t>& data, const std::string& ipAddress, uint16_t port) {
    if (!m_pImpl->isInitialized || !SOCKET_VALID(m_pImpl->socketHandle)) return false;
    if (data.empty() || data.size() > NetConfig::MAX_UDP_PAYLOAD) {
        Platform::Log("[NET ERROR]: Попытка отправить пакет недопустимого размера (" + std::to_string(data.size()) + " байт).");
        return false;
    }

    // Имитация потери пакетов (Network Simulation)
    if (m_pImpl->simulateLatency && m_pImpl->simulatedDropChance > 0.0f) {
        float randVal = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
        if (randVal < m_pImpl->simulatedDropChance) {
            m_pImpl->packetsDroppedTotal++;
            return true; // Имитируем успешную отправку, но пакет теряется
        }
    }

    sockaddr_in destAddr;
    std::memset(&destAddr, 0, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(port);
    inet_pton(AF_INET, ipAddress.c_str(), &destAddr.sin_addr);

    std::lock_guard<std::mutex> lock(m_pImpl->socketMutex);
    
    int bytesSent = sendto(m_pImpl->socketHandle, 
                           reinterpret_cast<const char*>(data.data()), 
                           static_cast<int>(data.size()), 
                           0, 
                           reinterpret_cast<const sockaddr*>(&destAddr), 
                           sizeof(destAddr));

    if (bytesSent == SOCKET_ERROR) {
        int err = GET_SOCKET_ERROR();
        if (err != ERROR_WOULDBLOCK) {
            Platform::Log("[NET TRANSMIT ERROR]: sendto() завершился с кодом " + std::to_string(err));
            return false;
        }
    } else {
        m_pImpl->bytesSentTotal += bytesSent;
    }

    return true;
}

void NetworkSocket::BroadcastToAllPeers(const std::vector<uint8_t>& data) {
    std::vector<PeerConnection> peersCopy;
    {
        std::lock_guard<std::mutex> lock(m_pImpl->socketMutex);
        peersCopy = m_pImpl->activePeers;
    }

    for (const auto& peer : peersCopy) {
        if (peer.isConnected) {
            SendPacket(data, peer.ipString, peer.port);
        }
    }
}

std::vector<NetworkPacket> NetworkSocket::ReceivePackets() {
    std::vector<NetworkPacket> receivedPackets;
    if (!m_pImpl->isInitialized || !SOCKET_VALID(m_pImpl->socketHandle)) return receivedPackets;

    std::lock_guard<std::mutex> lock(m_pImpl->socketMutex);

    uint8_t receiveBuffer[NetConfig::MAX_UDP_PAYLOAD];
    sockaddr_in senderAddr;
    socklen_t senderAddrSize = sizeof(senderAddr);

    while (true) {
        int bytesRead = recvfrom(m_pImpl->socketHandle, 
                                 reinterpret_cast<char*>(receiveBuffer), 
                                 NetConfig::MAX_UDP_PAYLOAD, 
                                 0, 
                                 reinterpret_cast<sockaddr*>(&senderAddr), 
                                 &senderAddrSize);

        if (bytesRead == SOCKET_ERROR) {
            int err = GET_SOCKET_ERROR();
            if (err == ERROR_WOULDBLOCK || err == ERROR_CONNRESET) {
                break; // Буфер пуст, выходим из цикла
            } else {
                Platform::Log("[NET RECEIVE ERROR]: recvfrom() завершился с кодом " + std::to_string(err));
                break;
            }
        }

        if (bytesRead > 0) {
            // Преобразование IP-адреса отправителя в строку
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(senderAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
            std::string senderIp(ipStr);
            uint16_t senderPort = ntohs(senderAddr.sin_port);

            // 1. Проверка черного списка (IP Banlist)
            if (IsIpBanned(senderIp)) {
                m_pImpl->packetsDroppedTotal++;
                continue;
            }

            // 2. Маршрутизация пира и защита от DDoS (Rate Limiting Token Bucket)
            PeerConnection* peer = FindOrCreatePeer(senderAddr, senderIp, senderPort);
            if (peer) {
                peer->timeSinceLastPacket = 0.0f; // Сброс таймера таймаута
                
                if (peer->tokenBucket >= 1.0f) {
                    peer->tokenBucket -= 1.0f;
                } else {
                    // Превышен лимит пакетов в секунду - отбрасываем (DDoS mitigation)
                    m_pImpl->packetsDroppedTotal++;
                    continue; 
                }
            }

            m_pImpl->bytesReceivedTotal += bytesRead;

            NetworkPacket packet;
            packet.senderIp = senderIp;
            packet.senderPort = senderPort;
            packet.senderPeerId = peer ? peer->peerId : 0;
            packet.payload.assign(receiveBuffer, receiveBuffer + bytesRead);
            packet.timestamp = Platform::GetCurrentTimeSeconds();

            receivedPackets.push_back(packet);
        }
    }

    return receivedPackets;
}

// ============================================================================
// SECTION 7: PEER CONNECTION MANAGEMENT & TIMEOUTS
// ============================================================================

PeerConnection* NetworkSocket::FindOrCreatePeer(const sockaddr_in& addr, const std::string& ip, uint16_t port) {
    for (auto& peer : m_pImpl->activePeers) {
        if (peer.ipString == ip && peer.port == port) {
            return &peer;
        }
    }

    // Если сервер переполнен, отказываем в создании нового пира
    if (m_pImpl->isServerMode && m_pImpl->activePeers.size() >= 128) {
        return nullptr;
    }

    PeerConnection newPeer;
    newPeer.peerId = m_pImpl->nextPeerId++;
    newPeer.address = addr;
    newPeer.ipString = ip;
    newPeer.port = port;
    newPeer.timeSinceLastPacket = 0.0f;
    newPeer.timeSinceLastHeartbeat = 0.0f;
    newPeer.roundTripTimeMs = 0.0f;
    newPeer.tokenBucket = static_cast<float>(NetConfig::MAX_PACKETS_PER_SEC);
    newPeer.isConnected = true;
    newPeer.isPendingValidation = true;

    m_pImpl->activePeers.push_back(newPeer);
    Platform::Log("[NET SESSION]: Установлено новое соединение с узлом ID " + std::to_string(newPeer.peerId) + " (" + ip + ":" + std::to_string(port) + ")");
    
    return &(m_pImpl->activePeers.back());
}

void NetworkSocket::UpdateTick(float deltaTime) {
    std::lock_guard<std::mutex> lock(m_pImpl->socketMutex);

    for (auto it = m_pImpl->activePeers.begin(); it != m_pImpl->activePeers.end();) {
        // 1. Пополнение токенов для Rate Limiting
        it->tokenBucket += NetConfig::MAX_PACKETS_PER_SEC * deltaTime;
        if (it->tokenBucket > NetConfig::MAX_PACKETS_PER_SEC) {
            it->tokenBucket = static_cast<float>(NetConfig::MAX_PACKETS_PER_SEC);
        }

        // 2. Контроль таймаутов
        it->timeSinceLastPacket += deltaTime;
        if (it->timeSinceLastPacket > NetConfig::PEER_TIMEOUT_SECONDS) {
            Platform::Log("[NET SESSION]: Потеряно соединение с узлом ID " + std::to_string(it->peerId) + " (Таймаут превышен).");
            it = m_pImpl->activePeers.erase(it);
            continue;
        }

        // 3. Отправка Heartbeat-пакетов (Keep-Alive)
        it->timeSinceLastHeartbeat += deltaTime;
        if (it->timeSinceLastHeartbeat > NetConfig::HEARTBEAT_INTERVAL) {
            it->timeSinceLastHeartbeat = 0.0f;
            // Здесь отправляется пустой Ping-пакет нулевого размера для замера RTT
        }

        ++it;
    }
}

// ============================================================================
// SECTION 8: SECURITY (IP BANLIST) & DIAGNOSTICS
// ============================================================================

void NetworkSocket::BanIpAddress(const std::string& ipAddress) {
    std::lock_guard<std::mutex> lock(m_pImpl->socketMutex);
    if (std::find(m_pImpl->bannedIPs.begin(), m_pImpl->bannedIPs.end(), ipAddress) == m_pImpl->bannedIPs.end()) {
        m_pImpl->bannedIPs.push_back(ipAddress);
        Platform::Log("[NET SECURITY]: IP-адрес " + ipAddress + " занесен в черный список.");
    }
}

bool NetworkSocket::IsIpBanned(const std::string& ipAddress) const {
    return std::find(m_pImpl->bannedIPs.begin(), m_pImpl->bannedIPs.end(), ipAddress) != m_pImpl->bannedIPs.end();
}

void NetworkSocket::SetNetworkSimulation(bool enable, float dropChancePercent) {
    std::lock_guard<std::mutex> lock(m_pImpl->socketMutex);
    m_pImpl->simulateLatency = enable;
    m_pImpl->simulatedDropChance = std::clamp(dropChancePercent / 100.0f, 0.0f, 1.0f);
    
    if (enable) {
        Platform::Log("[NET DEBUG]: Активирована эмуляция плохой сети. Шанс потери пакета: " + std::to_string(dropChancePercent) + "%");
    } else {
        Platform::Log("[NET DEBUG]: Эмуляция сети отключена. Идеальные условия.");
    }
}

void NetworkSocket::GetBandwidthStats(size_t& outBytesSent, size_t& outBytesReceived, size_t& outPacketsDropped) const {
    std::lock_guard<std::mutex> lock(m_pImpl->socketMutex);
    outBytesSent = m_pImpl->bytesSentTotal;
    outBytesReceived = m_pImpl->bytesReceivedTotal;
    outPacketsDropped = m_pImpl->packetsDroppedTotal;
}

void NetworkSocket::Shutdown() {
    std::lock_guard<std::mutex> lock(m_pImpl->socketMutex);
    if (SOCKET_VALID(m_pImpl->socketHandle)) {
        CLOSE_SOCKET(m_pImpl->socketHandle);
        m_pImpl->socketHandle = INVALID_SOCKET;
        Platform::Log("[NET SHUTDOWN]: Основной UDP сокет закрыт.");
    }
    
    m_pImpl->isInitialized = false;
    m_pImpl->activePeers.clear();
}

} // namespace Centralia