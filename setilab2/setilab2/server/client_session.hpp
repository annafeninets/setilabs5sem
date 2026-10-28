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
    void sendStatus(bool ok);
    std::filesystem::path publishFile(const std::filesystem::path& tempPath,
                                      const std::filesystem::path& wantedPath) const;

    TcpConnection connection_;
    std::filesystem::path uploadsDir_;
    std::string clientLabel_;

    std::atomic<uint64_t> receivedBytes_{0};
    uint64_t totalBytes_{0};
    bool statusSent_{false};
};
