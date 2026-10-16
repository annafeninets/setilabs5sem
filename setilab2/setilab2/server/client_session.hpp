#pragma once

#include "../common/tcp_connection.hpp"

#include <atomic>
#include <cstdint>
#include <filesystem>

class ClientSession {
public:
    ClientSession(TcpConnection connection, std::filesystem::path uploadsDir);

    void run();

private:
    std::filesystem::path resolveOutputPath(const std::string& clientFileName) const;

    void receiveAndStoreFile();

    TcpConnection connection_;
    std::filesystem::path uploadsDir_;
    std::string clientLabel_;

    std::atomic<uint64_t> receivedBytes_{0};
    uint64_t totalBytes_{0};
};
