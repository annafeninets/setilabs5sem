#include "server.hpp"

#include "../common/platform.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    int portArg;
    try {
        portArg = std::stoi(argv[1]);
    } catch (const std::exception&) {
        std::cerr << "Invalid port: " << argv[1] << "\n";
        return 1;
    }
    if (portArg <= 0 || portArg > 65535) {
        std::cerr << "Port out of range: " << portArg << "\n";
        return 1;
    }

    try {
        initializeSockets();
        Server server(static_cast<uint16_t>(portArg));
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        cleanupSockets();
        return 1;
    }
}
