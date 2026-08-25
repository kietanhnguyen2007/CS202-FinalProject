#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include "SurvivalServerCore.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

using SurvivalBackend::SurvivalServerCore;
using SurvivalBackend::ValidationError;

struct SocketGuard {
    SOCKET value = INVALID_SOCKET;
    ~SocketGuard() {
        if (value != INVALID_SOCKET) closesocket(value);
    }
};

struct HttpRequest {
    std::string method;
    std::string target;
    std::map<std::string, std::string> headers;
    std::string body;
};

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string trim(std::string value) {
    const auto notSpace = [](unsigned char c) { return std::isspace(c) == 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

bool receiveRequest(SOCKET socket, HttpRequest& request) {
    constexpr std::size_t maxRequestBytes = 1024 * 1024;
    std::string data;
    char buffer[8192];
    std::size_t headerEnd = std::string::npos;
    while ((headerEnd = data.find("\r\n\r\n")) == std::string::npos) {
        const int count = recv(socket, buffer, sizeof(buffer), 0);
        if (count <= 0) return false;
        data.append(buffer, static_cast<std::size_t>(count));
        if (data.size() > maxRequestBytes) throw std::runtime_error("request too large");
    }

    std::istringstream headerStream(data.substr(0, headerEnd));
    std::string requestLine;
    if (!std::getline(headerStream, requestLine)) return false;
    if (!requestLine.empty() && requestLine.back() == '\r') requestLine.pop_back();
    std::istringstream requestLineStream(requestLine);
    std::string version;
    if (!(requestLineStream >> request.method >> request.target >> version)) return false;

    std::string line;
    while (std::getline(headerStream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        request.headers[lower(trim(line.substr(0, colon)))] = trim(line.substr(colon + 1));
    }

    std::size_t contentLength = 0;
    const auto lengthHeader = request.headers.find("content-length");
    if (lengthHeader != request.headers.end()) {
        try {
            contentLength = static_cast<std::size_t>(std::stoull(lengthHeader->second));
        } catch (...) {
            throw std::runtime_error("invalid Content-Length");
        }
    }
    if (contentLength > maxRequestBytes) throw std::runtime_error("request too large");

    request.body = data.substr(headerEnd + 4);
    while (request.body.size() < contentLength) {
        const int count = recv(socket, buffer, sizeof(buffer), 0);
        if (count <= 0) return false;
        request.body.append(buffer, static_cast<std::size_t>(count));
    }
    request.body.resize(contentLength);
    return true;
}

const char* reasonPhrase(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 413: return "Payload Too Large";
        case 422: return "Unprocessable Entity";
        default: return "Internal Server Error";
    }
}

void sendAll(SOCKET socket, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const int count = send(socket, data.data() + sent,
            static_cast<int>(data.size() - sent), 0);
        if (count <= 0) return;
        sent += static_cast<std::size_t>(count);
    }
}

void sendJson(SOCKET socket, int status, const nlohmann::json& body) {
    const std::string payload = body.dump();
    std::ostringstream response;
    response << "HTTP/1.1 " << status << ' ' << reasonPhrase(status) << "\r\n"
             << "Content-Type: application/json; charset=utf-8\r\n"
             << "Content-Length: " << payload.size() << "\r\n"
             << "Connection: close\r\n"
             << "Cache-Control: no-store\r\n\r\n"
             << payload;
    sendAll(socket, response.str());
}

std::string header(const HttpRequest& request, const std::string& name) {
    const auto found = request.headers.find(lower(name));
    return found == request.headers.end() ? std::string{} : found->second;
}

std::string urlDecode(const std::string& value) {
    std::string decoded;
    decoded.reserve(value.size());
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            decoded.push_back(' ');
        } else if (value[i] == '%' && i + 2 < value.size()) {
            const std::string hex = value.substr(i + 1, 2);
            char* end = nullptr;
            const long code = std::strtol(hex.c_str(), &end, 16);
            if (end && *end == '\0') {
                decoded.push_back(static_cast<char>(code));
                i += 2;
            } else {
                decoded.push_back(value[i]);
            }
        } else {
            decoded.push_back(value[i]);
        }
    }
    return decoded;
}

std::map<std::string, std::string> parseQuery(const std::string& query) {
    std::map<std::string, std::string> values;
    std::size_t start = 0;
    while (start <= query.size()) {
        const std::size_t end = query.find('&', start);
        const std::string pair = query.substr(start, end - start);
        const std::size_t equals = pair.find('=');
        values[urlDecode(pair.substr(0, equals))] =
            equals == std::string::npos ? "" : urlDecode(pair.substr(equals + 1));
        if (end == std::string::npos) break;
        start = end + 1;
    }
    return values;
}

void routeRequest(SOCKET socket, const HttpRequest& request, SurvivalServerCore& core) {
    const std::size_t queryStart = request.target.find('?');
    const std::string path = request.target.substr(0, queryStart);
    const std::string query = queryStart == std::string::npos
        ? std::string{} : request.target.substr(queryStart + 1);

    if (request.method == "GET" && path == "/health") {
        sendJson(socket, 200, {{"status", "ok"}, {"runtime", "cpp17"}});
        return;
    }

    if (request.method == "POST" && path == "/v1/auth/guest") {
        nlohmann::json body = request.body.empty()
            ? nlohmann::json::object() : nlohmann::json::parse(request.body);
        if (!body.is_object()) throw ValidationError("request body must be an object");
        const std::string displayName = body.value("displayName", "Player");
        sendJson(socket, 201, core.createGuest(displayName));
        return;
    }

    if (request.method == "GET" && path == "/v1/players/me") {
        const std::string playerId = header(request, "X-Player-Id");
        if (playerId.empty()) throw ValidationError("X-Player-Id required");
        const auto result = core.profile(playerId);
        if (!result) sendJson(socket, 404, {{"detail", "player not found"}});
        else sendJson(socket, 200, *result);
        return;
    }

    const std::string runPrefix = "/v1/runs/";
    const std::string runSuffix = "/complete";
    if (request.method == "POST" && path.size() > runPrefix.size() + runSuffix.size()
        && path.compare(0, runPrefix.size(), runPrefix) == 0
        && path.compare(path.size() - runSuffix.size(), runSuffix.size(), runSuffix) == 0) {
        const std::string runId = path.substr(
            runPrefix.size(), path.size() - runPrefix.size() - runSuffix.size());
        const auto body = nlohmann::json::parse(request.body);
        const auto result = core.submitResult(
            header(request, "X-Player-Id"), runId,
            header(request, "Idempotency-Key"), body);
        sendJson(socket, 200, result);
        return;
    }

    if (request.method == "GET" && path == "/v1/leaderboards/score") {
        const auto values = parseQuery(query);
        std::optional<std::string> character;
        int limit = 50;
        if (const auto found = values.find("character"); found != values.end() && !found->second.empty()) {
            character = found->second;
        }
        if (const auto found = values.find("limit"); found != values.end() && !found->second.empty()) {
            try {
                limit = std::stoi(found->second);
            } catch (...) {
                throw ValidationError("limit must be an integer");
            }
        }
        sendJson(socket, 200, {{"board", "score"}, {"entries", core.leaderboard(character, limit)}});
        return;
    }

    sendJson(socket, 404, {{"detail", "route not found"}});
}

int parsePort(const char* value) {
    try {
        const int port = std::stoi(value);
        if (port < 1 || port > 65535) throw std::out_of_range("port");
        return port;
    } catch (...) {
        throw std::runtime_error("Port must be between 1 and 65535");
    }
}

} // namespace

int main(int argc, char** argv) {
    try {
        const int port = argc > 1 ? parsePort(argv[1]) : 8080;
        const std::string dataPath = argc > 2 ? argv[2] : "survival3d_data.json";

        WSADATA winsockData{};
        if (WSAStartup(MAKEWORD(2, 2), &winsockData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
        struct WinsockGuard { ~WinsockGuard() { WSACleanup(); } } winsockGuard;

        SocketGuard listener;
        listener.value = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listener.value == INVALID_SOCKET) throw std::runtime_error("Cannot create socket");

        const BOOL reuse = TRUE;
        setsockopt(listener.value, SOL_SOCKET, SO_REUSEADDR,
            reinterpret_cast<const char*>(&reuse), sizeof(reuse));

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_port = htons(static_cast<u_short>(port));
        inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
        if (bind(listener.value, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
            throw std::runtime_error("Cannot bind 127.0.0.1:" + std::to_string(port));
        }
        if (listen(listener.value, SOMAXCONN) == SOCKET_ERROR) {
            throw std::runtime_error("Cannot listen on socket");
        }

        SurvivalServerCore core(dataPath);
        std::cout << "Aegis Rift C++ server: http://127.0.0.1:" << port << '\n'
                  << "Data: " << dataPath << '\n'
                  << "Press Ctrl+C to stop.\n";

        while (true) {
            SocketGuard client;
            client.value = accept(listener.value, nullptr, nullptr);
            if (client.value == INVALID_SOCKET) continue;
            try {
                HttpRequest request;
                if (receiveRequest(client.value, request)) routeRequest(client.value, request, core);
            } catch (const ValidationError& error) {
                sendJson(client.value, 422, {{"detail", error.what()}});
            } catch (const nlohmann::json::exception& error) {
                sendJson(client.value, 400, {{"detail", std::string("invalid JSON: ") + error.what()}});
            } catch (const std::exception& error) {
                const std::string message = error.what();
                sendJson(client.value, message == "request too large" ? 413 : 500, {{"detail", message}});
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Server failed: " << error.what() << '\n';
        return 1;
    }
}
