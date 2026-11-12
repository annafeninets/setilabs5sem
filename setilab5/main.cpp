#include <signal.h>

#include <cstdio>
#include <cstdlib>
#include <exception>

#include "server.hpp"

namespace {
    constexpr long kMinPort = 1;
    constexpr long kMaxPort = 65535;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 2;
    }
    char* end = nullptr;
    long port = std::strtol(argv[1], &end, 10);
    if (*end != '\0' || port < kMinPort || port > kMaxPort) {
        std::fprintf(stderr, "invalid port: %s\n", argv[1]);
        return 2;
    }

    signal(SIGPIPE, SIG_IGN);
    try {
        Server server(static_cast<uint16_t>(port));
        server.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "fatal: %s\n", e.what());
        return 1;
    }
}