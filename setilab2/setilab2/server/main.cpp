#include "server.hpp"

#include "../common/platform.hpp"
#include "../common/port_parser.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    uint16_t port;
    if (!parsePort(argv[1], port)) {
        std::cerr << "Invalid port: " << argv[1] << "\n";
        return 1;
    }

    try {
        initializeSockets();
        Server server(port);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        cleanupSockets();
        return 1;
    }
}
