#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "platform/Platform.hpp" // Наш кроссплатформенный логер

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib") // МАСТЕР-ФИКС: Авто-линковка сетевого драйвера Windows в cl.exe
    using SocketType = SOCKET;
    #define CLOSE_SOCKET(s) closesocket(s)
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define SOCKET_ERROR_VAL SOCKET_ERROR
    using socklen_t = int;
#else // LINUX / ANDROID
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    using SocketType = int;
    #define CLOSE_SOCKET(s) ::close(s)
    #define INVALID_SOCKET_VAL -1
    #define SOCKET_ERROR_VAL -1
#endif

namespace Centralia {

class NetworkSocket {
private:
    SocketType m_socket;
    bool m_isListening;

public:
    inline NetworkSocket() noexcept : m_socket(INVALID_SOCKET_VAL), m_isListening(false) {}

    inline ~NetworkSocket() {
        Close();
    }

    // Запрет случайного копирования сокетов для предотвращения утечки системных дескрипторов
    NetworkSocket(const NetworkSocket&) = delete;
    NetworkSocket& operator=(const NetworkSocket&) = delete;

    // Инициализация сетевых платформ ОС (Winsock старт)
    inline static bool GlobalInit() noexcept {
#if defined(_WIN32)
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            Platform::Log("Network: Winsock initialization failed!");
            return false;
        }
#endif
        return true;
    }

    inline static void GlobalCleanup() noexcept {
#if defined(_WIN32)
        WSACleanup();
#endif
    }

    // Логика Хоста (Сервера Centralia)
    inline bool StartServer(uint16_t port) noexcept {
        m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (!IsValid()) {
            Platform::Log("Network: Failed to create listening socket.");
            return false;
        }

        // Позволяет повторно использовать порт сразу после перезапуска хоста игры
        int opt = 1;
#if defined(_WIN32)
        setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
        setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY; // Слушаем любые входящие IP-подключения игроков
        serverAddr.sin_port = htons(port);

        if (bind(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR_VAL) {
            Platform::Log("Network: Socket bind failed on port " + std::to_string(port));
            Close();
            return false;
        }

        if (listen(m_socket, SOMAXCONN) == SOCKET_ERROR_VAL) {
            Platform::Log("Network: Socket listen failed.");
            Close();
            return false;
        }

        m_isListening = true;
        Platform::Log("Network: Server hosted successfully on port " + std::to_string(port) + ". Waiting for players...");
        return true;
    }

    inline bool AcceptConnection(NetworkSocket& clientSocket) noexcept {
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
        Platform::Log("Network: New player connected from IP: " + std::string(ipStr));
        
        return true;
    }

    // Логика Клиента Centralia (Подключение к Хосту)
    inline bool ConnectToServer(const std::string& ipAddress, uint16_t port) noexcept {
        m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (!IsValid()) {
            Platform::Log("Network: Failed to create client socket.");
            return false;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        
        if (inet_pton(AF_INET, ipAddress.c_str(), &serverAddr.sin_addr) <= 0) {
            Platform::Log("Network: Invalid IP Address configuration: " + ipAddress);
            Close();
            return false;
        }

        if (connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR_VAL) {
            Platform::Log("Network: Connection to host " + ipAddress + ":" + std::to_string(port) + " failed.");
            Close();
            return false;
        }

        Platform::Log("Network: Successfully connected to Centralia host: " + ipAddress);
        return true;
    }

    // Общие методы отправки и получения сырых байтов
    inline bool SendBytes(const std::vector<uint8_t>& data) noexcept {
        if (!IsValid()) return false;
        
        size_t totalSent = 0;
        size_t bytesLeft = data.size();
        
        while (totalSent < data.size()) {
#if defined(_WIN32)
            int sent = send(m_socket, reinterpret_cast<const char*>(data.data() + totalSent), static_cast<int>(bytesLeft), 0);
#else
            ssize_t sent = send(m_socket, data.data() + totalSent, bytesLeft, 0);
#endif
            if (sent == SOCKET_ERROR_VAL) {
                Platform::Log("Network: transmission failure during send.");
                return false;
            }
            totalSent += static_cast<size_t>(sent);
            bytesLeft -= static_cast<size_t>(sent);
        }
        return true;
    }

    inline bool ReceiveBytes(std::vector<uint8_t>& outData, size_t maxBytes) noexcept {
        if (!IsValid()) return false;

        outData.resize(maxBytes);
#if defined(_WIN32)
        int bytesRead = recv(m_socket, reinterpret_cast<char*>(outData.data()), static_cast<int>(maxBytes), 0);
#else
        ssize_t bytesRead = recv(m_socket, outData.data(), maxBytes, 0);
#endif

        if (bytesRead <= 0) {
            outData.clear();
            return false; 
        }

        outData.resize(static_cast<size_t>(bytesRead));
        return true;
    }

    inline void Close() noexcept {
        if (IsValid()) {
            CLOSE_SOCKET(m_socket);
            m_socket = INVALID_SOCKET_VAL;
        }
        m_isListening = false;
    }

    [[nodiscard]] inline bool IsValid() const noexcept {
        return m_socket != INVALID_SOCKET_VAL;
    }
};

} // namespace Centralia
