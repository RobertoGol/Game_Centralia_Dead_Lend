#include "core/NetworkSocket.hpp"
#include "platform/Platform.hpp"
#include <cstring>

#if defined(CENTRALIA_PLATFORM_WINDOWS)
    #define CLOSE_SOCKET(s) closesocket(s)
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define SOCKET_ERROR_VAL SOCKET_ERROR
#else
    #define CLOSE_SOCKET(s) ::close(s)
    #define INVALID_SOCKET_VAL -1
    #define SOCKET_ERROR_VAL -1
#endif

namespace Centralia {

NetworkSocket::NetworkSocket() : m_socket(INVALID_SOCKET_VAL), m_isListening(false) {}

NetworkSocket::~NetworkSocket() {
    Close();
}

bool NetworkSocket::GlobalInit() {
#if defined(CENTRALIA_PLATFORM_WINDOWS)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Platform::Log("Winsock initialization failed!");
        return false;
    }
#endif
    return true;
}

void NetworkSocket::GlobalCleanup() {
#if defined(CENTRALIA_PLATFORM_WINDOWS)
    WSACleanup();
#endif
}

bool NetworkSocket::StartServer(uint16_t port) {
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!IsValid()) {
        Platform::Log("Failed to create listening socket.");
        return false;
    }

    // Позволяет повторно использовать порт сразу после перезапуска хоста
    int opt = 1;
#if defined(CENTRALIA_PLATFORM_WINDOWS)
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Слушаем любые входящие IP-подключения
    serverAddr.sin_port = htons(port);

    if (bind(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR_VAL) {
        Platform::Log("Socket bind failed on port " + std::to_string(port));
        Close();
        return false;
    }

    if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR_VAL) {
        Platform::Log("Socket listen failed.");
        Close();
        return false;
    }

    m_isListening = true;
    Platform::Log("Server hosted successfully on port " + std::to_string(port) + ". Waiting for players...");
    return true;
}

bool NetworkSocket::AcceptConnection(NetworkSocket& clientSocket) {
    if (!m_isListening) return false;

    sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);

    SocketType incoming = accept(m_socket, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
    if (incoming == INVALID_SOCKET_VAL) {
        return false;
    }

    clientSocket.Close();
    clientSocket.m_socket = incoming;
    
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    Platform::Log("New player connected from IP: " + std::string(ipStr));
    
    return true;
}

bool NetworkSocket::ConnectToServer(const std::string& ipAddress, uint16_t port) {
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (!IsValid()) {
        Platform::Log("Failed to create client socket.");
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr) <= 0) {
        Platform::Log("Invalid IP Address configuration.");
        Close();
        return false;
    }

    if (connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR_VAL) {
        Platform::Log("Connection to host " + ipAddress + ":" + std::to_string(port) + " failed.");
        Close();
        return false;
    }

    Platform::Log("Successfully connected to Centralia host: " + ipAddress);
    return true;
}

bool NetworkSocket::SendBytes(const std::vector<uint8_t>& data) {
    if (!IsValid()) return false;
    
    size_t totalSent = 0;
    size_t bytesLeft = data.size();
    
    while (totalSent < data.size()) {
#if defined(CENTRALIA_PLATFORM_WINDOWS)
        int sent = send(m_socket, reinterpret_cast<const char*>(data.data() + totalSent), static_cast<int>(bytesLeft), 0);
#else
        ssize_t sent = send(m_socket, data.data() + totalSent, bytesLeft, 0);
#endif
        if (sent == SOCKET_ERROR_VAL) {
            Platform::Log("Network transmission failure during send.");
            return false;
        }
        totalSent += sent;
        bytesLeft -= sent;
    }
    return true;
}

bool NetworkSocket::ReceiveBytes(std::vector<uint8_t>& outData, size_t maxBytes) {
    if (!IsValid()) return false;

    outData.resize(maxBytes);
#if defined(CENTRALIA_PLATFORM_WINDOWS)
    int bytesRead = recv(m_socket, reinterpret_cast<char*>(outData.data()), static_cast<int>(maxBytes), 0);
#else
    ssize_t bytesRead = recv(m_socket, outData.data(), maxBytes, 0);
#endif

    if (bytesRead <= 0) {
        outData.clear();
        return false; // Соединение разорвано или произошла ошибка
    }

    outData.resize(bytesRead);
    return true;
}

void NetworkSocket::Close() {
    if (IsValid()) {
        CLOSE_SOCKET(m_socket);
        m_socket = INVALID_SOCKET_VAL;
    }
    m_isListening = false;
}

bool NetworkSocket::IsValid() const {
    return m_socket != INVALID_SOCKET_VAL;
}

} // namespace Centralia
