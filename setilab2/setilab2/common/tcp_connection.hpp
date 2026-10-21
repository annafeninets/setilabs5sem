#pragma once

#include "socket.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

class TcpConnection {
public:
    explicit TcpConnection(Socket socket);

    TcpConnection(TcpConnection&&) noexcept = default;
    TcpConnection& operator=(TcpConnection&&) noexcept = default;
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    void sendAll(const void* data, size_t len);
    void recvAll(void* data, size_t len);

    size_t recvSome(void* buffer, size_t maxLen);

    void sendUint64(uint64_t value);
    uint64_t recvUint64();

    void sendString(const std::string& s);
    std::string recvString(uint64_t maxLen);

    std::string describeRemote() const;

    void setTimeout(int seconds);
    void shutdownSend() { ::shutdownSend(socket_.fd()); }

    native_socket_t fd() const { return socket_.fd(); }
    void close() { socket_.close(); }

private:
    Socket socket_;
};
