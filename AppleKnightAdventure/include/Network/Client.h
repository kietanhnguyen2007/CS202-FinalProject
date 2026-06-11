#ifndef CLIENT_H
#define CLIENT_H

#include "Packet.h"
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

class Client {
protected:
#ifdef _WIN32
    SOCKET m_socket;
#else
    int m_socket;
#endif
    std::string m_serverAddress;
    int m_port;
    bool m_connected;

public:
    Client();
    ~Client();

    bool Connect(const std::string& address, int port);
    void Disconnect();
    bool IsConnected() const;
    const std::string& GetServerAddress() const;
    int GetPort() const;

    bool Send(const Packet& packet);
    bool Receive(Packet& packet);

private:
    bool PlatformInit();
    void PlatformCleanup();
};

#endif
