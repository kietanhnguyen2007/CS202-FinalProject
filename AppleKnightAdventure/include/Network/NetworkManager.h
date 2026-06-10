#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include "Server.h"
#include "Client.h"
#include "Utils/Types.h"
#include <memory>

class NetworkManager {
protected:
    std::unique_ptr<Server> m_server;
    std::unique_ptr<Client> m_client;
    GameMode m_mode;

public:
    NetworkManager();
    ~NetworkManager();

    bool StartServer(int port);
    bool ConnectToServer(const std::string& address, int port);
    void Disconnect();
    GameMode GetMode() const;
    bool IsHost() const;
    bool IsConnected() const;

    bool SendToAll(const Packet& packet);
    bool SendToServer(const Packet& packet);
    bool Broadcast(const Packet& packet, int excludeClient = -1);
    bool ReceiveFromServer(Packet& packet);
    bool ReceiveFromClient(int clientIndex, Packet& packet);

    int GetClientCount() const;
    bool AcceptClients();
    void DisconnectClient(int clientIndex);

    void Update();
};

#endif
