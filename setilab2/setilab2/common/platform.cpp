#include "platform.hpp"

#include <stdexcept>

#ifdef _WIN32

void initializeSockets() {
    WSADATA wsaData;
    int rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (rc != 0) {
        throw std::runtime_error("WSAStartup failed with code " + std::to_string(rc));
    }
}

void cleanupSockets() { WSACleanup(); }

void closeNativeSocket(native_socket_t s) noexcept {
    if (s != kInvalidSocket) {
        ::closesocket(s);
    }
}

bool wasInterrupted() noexcept { return false; }

std::string lastSocketErrorMessage() {
    int err = WSAGetLastError();
    char buffer[256] = {0};
    DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                                static_cast<DWORD>(err), 0, buffer, sizeof(buffer), nullptr);
    std::string message = (len > 0) ? std::string(buffer, len) : std::string("unknown error");
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }
    return message + " (WSA error " + std::to_string(err) + ")";
}

#else

#include <cstring>

void initializeSockets() {}
void cleanupSockets() {}

void closeNativeSocket(native_socket_t s) noexcept {
    if (s != kInvalidSocket) {
        ::close(s);
    }
}

bool wasInterrupted() noexcept { return errno == EINTR; }

std::string lastSocketErrorMessage() { return std::strerror(errno); }

#endif
