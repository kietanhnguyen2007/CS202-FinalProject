#include "Network/NetworkManager.h"

NetworkManager::NetworkManager()
    : m_mode(GameMode::SinglePlayer)
{
}

NetworkManager::~NetworkManager() {
    Disconnect();
}

bool NetworkManager::StartServer(int port) {
    m_server = std::make_unique<Server>();
    if (!m_server->Start(port)) {
        m_server.reset();
        return false;
    }
    m_client.reset();
    m_mode = GameMode::MultiplayerHost;
    return true;
}

bool NetworkManager::ConnectToServer(const std::string& address, int port) {
    m_client = std::make_unique<Client>();
    if (!m_client->Connect(address, port)) {
        m_client.reset();
        return false;
    }
    m_server.reset();
    m_mode = GameMode::MultiplayerClient;
    return true;
}

void NetworkManager::Disconnect() {
    if (m_server) {
        m_server->Stop();
        m_server.reset();
    }
    if (m_client) {
        m_client->Disconnect();
        m_client.reset();
    }
    m_mode = GameMode::SinglePlayer;
}

GameMode NetworkManager::GetMode() const { return m_mode; }
bool NetworkManager::IsHost() const { return m_mode == GameMode::MultiplayerHost; }
bool NetworkManager::IsConnected() const {
    if (m_mode == GameMode::MultiplayerHost) return m_server && m_server->IsRunning();
    if (m_mode == GameMode::MultiplayerClient) return m_client && m_client->IsConnected();
    return false;
}

bool NetworkManager::SendToAll(const Packet& packet) {
    if (m_server && m_server->IsRunning()) {
        return m_server->Broadcast(packet);
    }
    return false;
}

bool NetworkManager::SendToServer(const Packet& packet) {
    if (m_client && m_client->IsConnected()) {
        return m_client->Send(packet);
    }
    return false;
}

bool NetworkManager::Broadcast(const Packet& packet, int excludeClient) {
    if (m_server && m_server->IsRunning()) {
        return m_server->Broadcast(packet, excludeClient);
    }
    return false;
}

bool NetworkManager::ReceiveFromServer(Packet& packet) {
    if (m_client && m_client->IsConnected()) {
        return m_client->Receive(packet);
    }
    return false;
}

bool NetworkManager::ReceiveFromClient(int clientIndex, Packet& packet) {
    if (m_server && m_server->IsRunning()) {
        return m_server->ReceiveFromClient(clientIndex, packet);
    }
    return false;
}

int NetworkManager::GetClientCount() const {
    if (m_server && m_server->IsRunning()) {
        return m_server->GetClientCount();
    }
    return 0;
}

bool NetworkManager::AcceptClients() {
    if (m_server && m_server->IsRunning()) {
        return m_server->AcceptClient();
    }
    return false;
}

void NetworkManager::DisconnectClient(int clientIndex) {
    if (m_server && m_server->IsRunning()) {
        m_server->DisconnectClient(clientIndex);
    }
}

void NetworkManager::Update() {
    if (m_server && m_server->IsRunning()) {
        m_server->AcceptClient();
    }
}
