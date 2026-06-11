#ifndef SERVER_H
#define SERVER_H

#include "Packet.h"
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

class Server {
protected:
#ifdef _WIN32
    SOCKET m_listenSocket;
    std::vector<SOCKET> m_clients;
#else
    int m_listenSocket;
    std::vector<int> m_clients;
#endif
    int m_port;
    int m_maxClients;
    bool m_running;

public:
    Server();
    ~Server();

    bool Start(int port, int maxClients = 1);
    void Stop();
    bool IsRunning() const;
    int GetPort() const;

    bool AcceptClient();
    void DisconnectClient(int clientIndex);
    void DisconnectAll();
    int GetClientCount() const;

    bool SendToClient(int clientIndex, const Packet& packet);
    bool Broadcast(const Packet& packet, int excludeClient = -1);
    bool ReceiveFromClient(int clientIndex, Packet& packet);

private:
    bool PlatformInit();
    void PlatformCleanup();
};

#endif
