#pragma once

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>

    using native_socket_t = SOCKET;
    inline constexpr native_socket_t kInvalidSocket = INVALID_SOCKET;
#else
    #include <arpa/inet.h>
    #include <cerrno>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>

    using native_socket_t = int;
    inline constexpr native_socket_t kInvalidSocket = -1;
#endif

#include <string>

void initializeSockets();
void cleanupSockets();
void closeNativeSocket(native_socket_t s) noexcept;
bool wasInterrupted() noexcept;
std::string lastSocketErrorMessage();
