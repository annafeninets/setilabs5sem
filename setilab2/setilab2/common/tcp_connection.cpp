#include "tcp_connection.hpp"
#include "connection_error.hpp"

#include <algorithm>
#include <climits>

namespace {
constexpr size_t kMaxSingleCall = 1 << 20;
}

TcpConnection::TcpConnection(Socket socket) : socket_(std::move(socket)) {}

void TcpConnection::sendAll(const void* data, size_t len) {
    const char* ptr = static_cast<const char*>(data);
    size_t sent = 0;
    while (sent < len) {
        int chunk = static_cast<int>(std::min(len - sent, kMaxSingleCall));
#ifdef MSG_NOSIGNAL
        int n = static_cast<int>(::send(socket_.fd(), ptr + sent, chunk, MSG_NOSIGNAL));
#else
        int n = static_cast<int>(::send(socket_.fd(), ptr + sent, chunk, 0));
#endif
        if (n < 0) {
            if (wasInterrupted()) continue;
            throw ConnectionError("send() failed: " + lastSocketErrorMessage());
        }
        if (n == 0) {
            throw ConnectionError("send() sent 0 bytes unexpectedly");
        }
        sent += static_cast<size_t>(n);
    }
}

size_t TcpConnection::recvSome(void* buffer, size_t maxLen) {
    int chunk = static_cast<int>(std::min(maxLen, kMaxSingleCall));
    while (true) {
        int n = static_cast<int>(::recv(socket_.fd(), static_cast<char*>(buffer), chunk, 0));
        if (n < 0) {
            if (wasInterrupted()) continue;
            throw ConnectionError("recv() failed: " + lastSocketErrorMessage());
        }
        return static_cast<size_t>(n);
    }
}

void TcpConnection::recvAll(void* data, size_t len) {
    char* ptr = static_cast<char*>(data);
    size_t received = 0;
    while (received < len) {
        size_t n = recvSome(ptr + received, len - received);
        if (n == 0) {
            throw ConnectionError("peer closed connection prematurely");
        }
        received += n;
    }
}

void TcpConnection::sendUint64(uint64_t value) {
    uint8_t bytes[8];
    for (int i = 0; i < 8; ++i) {
        bytes[i] = static_cast<uint8_t>(value >> (8 * (7 - i)));
    }
    sendAll(bytes, sizeof(bytes));
}

uint64_t TcpConnection::recvUint64() {
    uint8_t bytes[8];
    recvAll(bytes, sizeof(bytes));
    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value = (value << 8) | bytes[i];
    }
    return value;
}

void TcpConnection::sendString(const std::string& s) {
    sendUint64(static_cast<uint64_t>(s.size()));
    if (!s.empty()) {
        sendAll(s.data(), s.size());
    }
}

std::string TcpConnection::recvString(uint64_t maxLen) {
    uint64_t len = recvUint64();
    if (len > maxLen) {
        throw ConnectionError("peer declared a string longer than the allowed maximum");
    }
    std::string s(static_cast<size_t>(len), '\0');
    if (len > 0) {
        recvAll(&s[0], s.size());
    }
    return s;
}

std::string TcpConnection::describeRemote() const {
    sockaddr_storage addr{};
    socklen_t addrLen = sizeof(addr);
    if (getpeername(socket_.fd(), reinterpret_cast<sockaddr*>(&addr), &addrLen) != 0) {
        return "<unknown peer>";
    }

    char host[NI_MAXHOST] = {0};
    char service[NI_MAXSERV] = {0};
    if (getnameinfo(reinterpret_cast<sockaddr*>(&addr), addrLen, host, sizeof(host), service,
                     sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV) != 0) {
        return "<unknown peer>";
    }
    return std::string(host) + ":" + service;
}
