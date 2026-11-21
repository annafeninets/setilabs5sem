#include <signal.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>
#include <vector>

#include "credentials.hpp"
#include "dns.hpp"
#include "server.hpp"

namespace {
    constexpr long kMinPort = 1;
    constexpr long kMaxPort = 65535;
    constexpr size_t kPortOnly = 1;
    constexpr size_t kPortAndResolver = 2;
    constexpr size_t kPortIndex = 0;
    constexpr size_t kResolverIndex = 1;
    constexpr const char* kAuthOption = "--auth";
    constexpr int kExitUsage = 2;
    constexpr int kExitFatal = 1;

    void printUsage(const char* program) {
        std::fprintf(stderr, "usage: %s <port> [<dns_ipv4>:<dns_port>] [--auth <user>:<password>]...\n", program);
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> positional;
    CredentialStore credentials;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], kAuthOption) != 0) {
            positional.emplace_back(argv[i]);
            continue;
        }
        if (i + 1 >= argc) {
            std::fprintf(stderr, "%s requires a value\n", kAuthOption);
            printUsage(argv[0]);
            return kExitUsage;
        }
        if (!credentials.addFromString(argv[++i])) {
            std::fprintf(stderr,
                         "invalid %s value (expected <user>:<password>, 1..255 bytes each, user must be unique)\n",
                         kAuthOption);
            return kExitUsage;
        }
    }
    if (positional.size() != kPortOnly && positional.size() != kPortAndResolver) {
        printUsage(argv[0]);
        return kExitUsage;
    }

    const std::string& portText = positional[kPortIndex];
    char* end = nullptr;
    long port = std::strtol(portText.c_str(), &end, 10);
    if (portText.empty() || *end != '\0' || port < kMinPort || port > kMaxPort) {
        std::fprintf(stderr, "invalid port: %s\n", portText.c_str());
        return kExitUsage;
    }
    sockaddr_in resolver{};
    if (positional.size() == kPortAndResolver) {
        if (!dns::parseResolverAddress(positional[kResolverIndex], resolver)) {
            std::fprintf(stderr, "invalid DNS resolver address: %s (expected <ipv4>:<port>)\n",
                         positional[kResolverIndex].c_str());
            return kExitUsage;
        }
    } else {
        resolver = dns::systemResolver();
    }

    signal(SIGPIPE, SIG_IGN);
    try {
        Server server(static_cast<uint16_t>(port), resolver, std::move(credentials));
        server.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "fatal: %s\n", e.what());
        return kExitFatal;
    }
}