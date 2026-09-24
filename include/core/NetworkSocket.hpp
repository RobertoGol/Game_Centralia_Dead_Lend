#pragma once
#include <string>
#include <vector>
#include <cstdint>

#if defined(CENTRALIA_PLATFORM_WINDOWS)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using SocketType = SOCKET;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    using SocketType = int;
#endif

namespace Centralia {

class NetworkSocket {
private:
    SocketType m_socket;
    bool m_isListening;

public:
    NetworkSocket();
    ~NetworkSocket();

    // Общая инициализация сетевых платформ (нужна в основном для Windows)
    static bool GlobalInit();
    static void GlobalCleanup();

    // Логика Хоста (Сервера)
    bool StartServer(uint16_t port);
    bool AcceptConnection(NetworkSocket& clientSocket);

    // Логика Клиента
    bool ConnectToServer(const std::string& ipAddress, uint16_t port);

    // Общие методы отправки и получения сырых байтов
    bool SendBytes(const std::vector<uint8_t>& data);
    bool ReceiveBytes(std::vector<uint8_t>& outData, size_t maxBytes);

    void Close();
    bool IsValid() const;
};

} // namespace Centralia
