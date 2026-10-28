#include "file_client.hpp"

#include "../common/platform.hpp"
#include "../common/port_parser.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

    void printUsage(const char* argv0) {
        std::cerr << "Usage: " << argv0 << " <file-path> <server-host> <server-port>\n";
    }

}

int main(int argc, char** argv) {
    if (argc != 4) {
        printUsage(argv[0]);
        return 1;
    }

    const std::string filePath = argv[1];
    const std::string host = argv[2];

    uint16_t port;
    if (!parsePort(argv[3], port)) {
        std::cerr << "Invalid port: " << argv[3] << "\n";
        return 1;
    }

    try {
        initializeSockets();
        FileClient client(host, port);
        bool ok = client.sendFile(filePath);

        std::cout << (ok ? "SUCCESS" : "FAIL") << std::endl;

        cleanupSockets();

        return ok ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cout << "FAIL" << std::endl;
        cleanupSockets();
        return 1;
    }
}
