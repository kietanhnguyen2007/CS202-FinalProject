#include "Network/Client.h"
#include "Utils/Constants.h"

Client::Client()
    : m_port(0)
    , m_connected(false)
{
#ifdef _WIN32
    m_socket = INVALID_SOCKET;
#else
    m_socket = -1;
#endif
}

Client::~Client() {
    Disconnect();
}

bool Client::Connect(const std::string& address, int port) {
    if (m_connected) Disconnect();
    if (!PlatformInit()) return false;

#ifdef _WIN32
    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) return false;
#else
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) return false;
#endif

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<unsigned short>(port));
    serverAddr.sin_addr.s_addr = inet_addr(address.c_str());

    if (connect(m_socket, reinterpret_cast<sockaddr*>(&serverAddr),
                sizeof(serverAddr)) < 0) {
        Disconnect();
        return false;
    }

    m_serverAddress = address;
    m_port = port;
    m_connected = true;
    return true;
}

void Client::Disconnect() {
    m_connected = false;
    m_serverAddress.clear();
    m_port = 0;

#ifdef _WIN32
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
#else
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
    }
#endif

    PlatformCleanup();
}

bool Client::IsConnected() const { return m_connected; }
const std::string& Client::GetServerAddress() const { return m_serverAddress; }
int Client::GetPort() const { return m_port; }

bool Client::Send(const Packet& packet) {
    if (!m_connected) return false;
    int sent = send(m_socket, packet.GetData(),
                    static_cast<int>(packet.GetSize()), 0);
    return sent == static_cast<int>(packet.GetSize());
}

bool Client::Receive(Packet& packet) {
    if (!m_connected) return false;

    char buffer[NETWORK_BUFFER_SIZE];
    int received = recv(m_socket, buffer, NETWORK_BUFFER_SIZE, 0);
    if (received <= 0) {
        Disconnect();
        return false;
    }

    packet = Packet(std::vector<char>(buffer, buffer + received));
    return true;
}

bool Client::PlatformInit() {
#ifdef _WIN32
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
#else
    return true;
#endif
}

void Client::PlatformCleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}
