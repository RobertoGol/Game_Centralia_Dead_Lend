#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "platform/Platform.hpp"

#if defined(_WIN32)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    using SocketType = SOCKET;
    #define CLOSE_SOCKET(s) closesocket(s)
    #define INVALID_SOCKET_VAL INVALID_SOCKET
    #define SOCKET_ERROR_VAL SOCKET_ERROR
    using socklen_t = int;
#else
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
    NetworkSocket();
    ~NetworkSocket();

    NetworkSocket(const NetworkSocket&) = delete;
    NetworkSocket& operator=(const NetworkSocket&) = delete;

    static bool GlobalInit();
    static void GlobalCleanup();
    bool StartServer(uint16_t port);
    bool AcceptConnection(NetworkSocket& clientSocket);
    bool ConnectToServer(const std::string& ipAddress, uint16_t port);
    bool SendBytes(const std::vector<uint8_t>& data);
    bool ReceiveBytes(std::vector<uint8_t>& outData, size_t maxBytes);
    void Close();
    [[nodiscard]] bool IsValid() const;
};

} // namespace Centralia