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

void setSocketTimeouts(native_socket_t s, int seconds) {
    DWORD ms = static_cast<DWORD>(seconds) * 1000;
    if (::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&ms), sizeof(ms)) != 0 ||
        ::setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&ms), sizeof(ms)) != 0) {
        throw std::runtime_error("setsockopt(SO_RCVTIMEO/SO_SNDTIMEO) failed: " +
                                  lastSocketErrorMessage());
    }
}

bool wasTimedOut() noexcept { return WSAGetLastError() == WSAETIMEDOUT; }

void shutdownSend(native_socket_t s) noexcept { ::shutdown(s, SD_SEND); }

size_t maxFileNameBytesInDir(const std::string&) { return 255; }

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

void setSocketTimeouts(native_socket_t s, int seconds) {
    timeval tv{};
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    if (::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0 ||
        ::setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0) {
        throw std::runtime_error("setsockopt(SO_RCVTIMEO/SO_SNDTIMEO) failed: " +
                                  lastSocketErrorMessage());
    }
}

bool wasTimedOut() noexcept { return errno == EAGAIN || errno == EWOULDBLOCK; }

void shutdownSend(native_socket_t s) noexcept { ::shutdown(s, SHUT_WR); }

size_t maxFileNameBytesInDir(const std::string& dir) {
    long limit = ::pathconf(dir.c_str(), _PC_NAME_MAX);
    return limit > 0 ? static_cast<size_t>(limit) : 255;
}

std::string lastSocketErrorMessage() { return std::strerror(errno); }

#endif
