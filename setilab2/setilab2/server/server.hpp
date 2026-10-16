#pragma once

#include "../common/socket.hpp"

#include <atomic>
#include <cstdint>
#include <filesystem>

class Server {
public:
    explicit Server(uint16_t port, std::filesystem::path uploadsDir = "uploads");

    [[noreturn]] void run();

private:
    void setupListeningSocket();

    uint16_t port_;
    std::filesystem::path uploadsDir_;
    Socket listenSocket_;
    std::atomic<int> activeClients_{0};
};
