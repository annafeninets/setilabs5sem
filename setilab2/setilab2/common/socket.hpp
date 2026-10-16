#pragma once

#include "platform.hpp"

class Socket {
public:
    Socket() noexcept : handle_(kInvalidSocket) {}
    explicit Socket(native_socket_t handle) noexcept : handle_(handle) {}

    ~Socket() { close(); }

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    Socket(Socket&& other) noexcept : handle_(other.handle_) { other.handle_ = kInvalidSocket; }

    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            close();
            handle_ = other.handle_;
            other.handle_ = kInvalidSocket;
        }
        return *this;
    }

    native_socket_t fd() const noexcept { return handle_; }
    bool valid() const noexcept { return handle_ != kInvalidSocket; }

    void close() noexcept {
        if (handle_ != kInvalidSocket) {
            closeNativeSocket(handle_);
            handle_ = kInvalidSocket;
        }
    }

private:
    native_socket_t handle_;
};
