#include "Network/Server.h"
#include "Utils/Constants.h"

Server::Server()
    : m_port(0)
    , m_running(false)
{
#ifdef _WIN32
    m_listenSocket = INVALID_SOCKET;
#else
    m_listenSocket = -1;
#endif
}

Server::~Server() {
    Stop();
}

bool Server::Start(int port) {
    if (m_running) Stop();
    if (!PlatformInit()) return false;

#ifdef _WIN32
    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET) return false;
#else
    m_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenSocket < 0) return false;
#endif

    int opt = 1;
    setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<unsigned short>(port));

    if (bind(m_listenSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        Stop();
        return false;
    }

    if (listen(m_listenSocket, 4) < 0) {
        Stop();
        return false;
    }

    m_port = port;
    m_running = true;
    return true;
}

void Server::Stop() {
    m_running = false;
    DisconnectAll();

#ifdef _WIN32
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
#else
    if (m_listenSocket >= 0) {
        close(m_listenSocket);
        m_listenSocket = -1;
    }
#endif

    PlatformCleanup();
}

bool Server::IsRunning() const { return m_running; }
int Server::GetPort() const { return m_port; }

bool Server::AcceptClient() {
    if (!m_running) return false;

    sockaddr_in clientAddr;
#ifdef _WIN32
    int addrLen = sizeof(clientAddr);
    SOCKET clientSocket = accept(m_listenSocket,
        reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
    if (clientSocket == INVALID_SOCKET) return false;
#else
    socklen_t addrLen = sizeof(clientAddr);
    int clientSocket = accept(m_listenSocket,
        reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
    if (clientSocket < 0) return false;
#endif

    m_clients.push_back(clientSocket);
    return true;
}

void Server::DisconnectClient(int clientIndex) {
    if (clientIndex < 0 || clientIndex >= static_cast<int>(m_clients.size())) return;

#ifdef _WIN32
    closesocket(m_clients[clientIndex]);
#else
    close(m_clients[clientIndex]);
#endif

    m_clients.erase(m_clients.begin() + clientIndex);
}

void Server::DisconnectAll() {
    for (size_t i = 0; i < m_clients.size(); ++i) {
#ifdef _WIN32
        closesocket(m_clients[i]);
#else
        close(m_clients[i]);
#endif
    }
    m_clients.clear();
}

int Server::GetClientCount() const {
    return static_cast<int>(m_clients.size());
}

bool Server::SendToClient(int clientIndex, const Packet& packet) {
    if (!m_running) return false;
    if (clientIndex < 0 || clientIndex >= static_cast<int>(m_clients.size())) return false;

    int sent = send(m_clients[clientIndex], packet.GetData(),
                    static_cast<int>(packet.GetSize()), 0);
    return sent == static_cast<int>(packet.GetSize());
}

bool Server::Broadcast(const Packet& packet, int excludeClient) {
    if (!m_running) return false;
    bool allOk = true;
    for (size_t i = 0; i < m_clients.size(); ++i) {
        if (static_cast<int>(i) == excludeClient) continue;
        int sent = send(m_clients[i], packet.GetData(),
                        static_cast<int>(packet.GetSize()), 0);
        if (sent != static_cast<int>(packet.GetSize())) allOk = false;
    }
    return allOk;
}

bool Server::ReceiveFromClient(int clientIndex, Packet& packet) {
    if (!m_running) return false;
    if (clientIndex < 0 || clientIndex >= static_cast<int>(m_clients.size())) return false;

    char buffer[NETWORK_BUFFER_SIZE];
    int received = recv(m_clients[clientIndex], buffer, NETWORK_BUFFER_SIZE, 0);
    if (received <= 0) return false;

    packet = Packet(std::vector<char>(buffer, buffer + received));
    return true;
}

bool Server::PlatformInit() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void Server::PlatformCleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}
