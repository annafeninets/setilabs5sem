#include "file_client.hpp"

#include "../common/connection_error.hpp"
#include "../common/platform.hpp"
#include "../common/protocol_limits.hpp"
#include "../common/socket.hpp"
#include "../common/tcp_connection.hpp"
#include "../common/utf8.hpp"

#include <fstream>
#include <iostream>
#include <vector>

namespace {
constexpr size_t kChunkSize = 1 << 16; // 64 KiB
}

FileClient::FileClient(std::string host, uint16_t port)
    : host_(std::move(host)), port_(port) {}

TcpConnection FileClient::connectToServer() const {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* resolved = nullptr;
    std::string portStr = std::to_string(port_);
    int rc = getaddrinfo(host_.c_str(), portStr.c_str(), &hints, &resolved);
    if (rc != 0 || resolved == nullptr) {
        throw ConnectionError("could not resolve " + host_ + ":" + portStr +
                               " (getaddrinfo code " + std::to_string(rc) + ")");
    }

    Socket sock;
    for (addrinfo* p = resolved; p != nullptr; p = p->ai_next) {
        native_socket_t s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == kInvalidSocket) continue;
        if (::connect(s, p->ai_addr, static_cast<socklen_t>(p->ai_addrlen)) == 0) {
            sock = Socket(s);
            break;
        }
        closeNativeSocket(s);
    }
    freeaddrinfo(resolved);

    if (!sock.valid()) {
        throw ConnectionError("could not connect to " + host_ + ":" + portStr);
    }
    return TcpConnection(std::move(sock));
}

bool FileClient::sendFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("cannot open file: " + filePath.string());
    }

    const uint64_t fileSize = std::filesystem::file_size(filePath);
    if (fileSize > kMaxFileSizeBytes) {
        throw std::runtime_error("file is larger than the 1 TiB protocol limit: " +
                                  filePath.string());
    }

    const std::string fileName = filePath.filename().string();
    if (fileName.size() > kMaxFileNameBytes) {
        throw std::runtime_error("file name exceeds " + std::to_string(kMaxFileNameBytes) +
                                  " bytes (UTF-8): " + filePath.string());
    }
    if (!isValidUtf8(fileName)) {
        throw std::runtime_error("file name is not valid UTF-8: " + filePath.string());
    }

    TcpConnection conn = connectToServer();
    std::cout << "Connected to " << conn.describeRemote() << std::endl;

    conn.sendString(fileName);
    conn.sendUint64(fileSize);

    std::vector<char> buffer(kChunkSize);
    uint64_t sent = 0;
    while (sent < fileSize) {
        std::streamsize toRead =
            static_cast<std::streamsize>(std::min<uint64_t>(kChunkSize, fileSize - sent));
        file.read(buffer.data(), toRead);
        std::streamsize got = file.gcount();
        if (got <= 0) {
            throw std::runtime_error("unexpected EOF/error reading " + filePath.string());
        }
        conn.sendAll(buffer.data(), static_cast<size_t>(got));
        sent += static_cast<uint64_t>(got);
    }

    const std::string result = conn.recvString(64);
    return result == "SUCCESS";
}
