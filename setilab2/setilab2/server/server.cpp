#include "server.hpp"

#include "../common/connection_error.hpp"
#include "../common/logger.hpp"
#include "../common/platform.hpp"
#include "../common/protocol_limits.hpp"
#include "../common/tcp_connection.hpp"
#include "client_session.hpp"

#include <chrono>
#include <thread>

namespace {
constexpr int kMaxConcurrentClients = 10;
constexpr std::chrono::milliseconds kAcceptRetryDelay{100};
}

Server::Server(uint16_t port, std::filesystem::path uploadsDir)
    : port_(port), uploadsDir_(std::move(uploadsDir)) {
    setupListeningSocket();
}

void Server::setupListeningSocket() {
    native_socket_t s = ::socket(AF_INET, SOCK_STREAM, 0);
    if (s == kInvalidSocket) {
        throw ConnectionError("socket() failed: " + lastSocketErrorMessage());
    }
    listenSocket_ = Socket(s);

    int reuse = 1;
    ::setsockopt(listenSocket_.fd(), SOL_SOCKET, SO_REUSEADDR,
                 reinterpret_cast<const char*>(&reuse), sizeof(reuse));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (::bind(listenSocket_.fd(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        throw ConnectionError("bind() failed: " + lastSocketErrorMessage());
    }
    if (::listen(listenSocket_.fd(), /*backlog=*/64) != 0) {
        throw ConnectionError("listen() failed: " + lastSocketErrorMessage());
    }

    std::filesystem::create_directories(uploadsDir_);
    Logger::log("Listening on port ", port_, ", saving uploads to '", uploadsDir_.string(), "'");
}

void Server::run() {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);
        native_socket_t clientSocket = ::accept(
            listenSocket_.fd(), reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
        if (clientSocket == kInvalidSocket) {
            Logger::log("accept() failed: ", lastSocketErrorMessage());
            std::this_thread::sleep_for(kAcceptRetryDelay);
            continue;
        }

        if (activeClients_.load(std::memory_order_relaxed) >= kMaxConcurrentClients) {
            Logger::log("rejecting connection: concurrent client limit (",
                        kMaxConcurrentClients, ") reached");
            TcpConnection rejected{Socket(clientSocket)};
            try {
                rejected.setTimeout(1);
                rejected.sendString(kStatusFail);
                rejected.shutdownSend();
            } catch (const std::exception&) {
            }
            continue;
        }
        activeClients_.fetch_add(1, std::memory_order_relaxed);

        TcpConnection connection{Socket(clientSocket)};
        std::filesystem::path uploadsDir = uploadsDir_;

        try {
            std::thread([this, conn = std::move(connection), uploadsDir]() mutable {
                try {
                    ClientSession session(std::move(conn), uploadsDir);
                    session.run();
                } catch (...) {
                }
                activeClients_.fetch_sub(1, std::memory_order_relaxed);
            }).detach();
        } catch (const std::exception& e) {
            activeClients_.fetch_sub(1, std::memory_order_relaxed);
            Logger::log("cannot start client thread: ", e.what());
            std::this_thread::sleep_for(kAcceptRetryDelay);
        }
    }
}
