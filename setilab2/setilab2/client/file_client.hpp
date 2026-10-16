#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

class TcpConnection;
class FileClient {
public:
    FileClient(std::string host, uint16_t port);

    bool sendFile(const std::filesystem::path& filePath);

private:
    TcpConnection connectToServer() const;

    std::string host_;
    uint16_t port_;
};
