#include "file_client.hpp"

#include "../common/platform.hpp"

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

    int portArg;
    try {
        portArg = std::stoi(argv[3]);
    } catch (const std::exception&) {
        std::cerr << "Invalid port: " << argv[3] << "\n";
        return 1;
    }
    if (portArg <= 0 || portArg > 65535) {
        std::cerr << "Port out of range: " << portArg << "\n";
        return 1;
    }

    try {
        initializeSockets();
        FileClient client(host, static_cast<uint16_t>(portArg));
        bool ok = client.sendFile(filePath);

        std::cout << (ok ? "SUCCESS" : "FAIL") << std::endl;

        cleanupSockets();

        return ok ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        cleanupSockets();
        return 1;
    }
}
