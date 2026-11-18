#include <signal.h>

#include <cstdio>
#include <cstdlib>
#include <exception>

#include "dns.hpp"
#include "server.hpp"

namespace {
    constexpr long kMinPort = 1;
    constexpr long kMaxPort = 65535;
    constexpr int kArgcPortOnly = 2;
    constexpr int kArgcWithResolver = 3;
    constexpr int kPortArg = 1;
    constexpr int kResolverArg = 2;
}

int main(int argc, char** argv) {
    if (argc != kArgcPortOnly && argc != kArgcWithResolver) {
        std::fprintf(stderr, "usage: %s <port> [<dns_ipv4>:<dns_port>]\n", argv[0]);
        return 2;
    }
    char* end = nullptr;
    long port = std::strtol(argv[kPortArg], &end, 10);
    if (*end != '\0' || port < kMinPort || port > kMaxPort) {
        std::fprintf(stderr, "invalid port: %s\n", argv[kPortArg]);
        return 2;
    }
    sockaddr_in resolver{};
    if (argc == kArgcWithResolver) {
        if (!dns::parseResolverAddress(argv[kResolverArg], resolver)) {
            std::fprintf(stderr, "invalid DNS resolver address: %s (expected <ipv4>:<port>)\n",
                         argv[kResolverArg]);
            return 2;
        }
    } else {
        resolver = dns::systemResolver();
    }

    signal(SIGPIPE, SIG_IGN);
    try {
        Server server(static_cast<uint16_t>(port), resolver);
        server.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "fatal: %s\n", e.what());
        return 1;
    }
}