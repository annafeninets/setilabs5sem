#include "server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <system_error>

namespace {
[[noreturn]] void fail(const char* what) {
    throw std::system_error(errno, std::generic_category(), what);
}
}

Server::Server(uint16_t port)
    : listenFd_(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)),
      listenWatch_(epoll_, *this),
      dns_(epoll_) {
    if (!listenFd_.valid()) fail("socket");
    int one = 1;
    setsockopt(listenFd_.get(), SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(INADDR_ANY);
    sa.sin_port = htons(port);
    if (bind(listenFd_.get(), reinterpret_cast<sockaddr*>(&sa), sizeof sa) < 0) fail("bind");
    if (listen(listenFd_.get(), SOMAXCONN) < 0) fail("listen");
    listenWatch_.set(listenFd_.get(), EPOLLIN);
    std::fprintf(stderr, "SOCKS5 proxy listening on port %u\n", port);
}

void Server::run() {
    for (;;) {
        int timeout = dns_.processTimeouts();
        epoll_.poll(timeout);
        graveyard_.clear();
    }
}

void Server::onEvent(uint32_t) {
    for (;;) {
        int fd = accept4(listenFd_.get(), nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            if (errno == EINTR || errno == ECONNABORTED) continue;
            std::fprintf(stderr, "ERROR: accept: %s\n", std::strerror(errno));
            if (!sessions_.empty()) {
                listenWatch_.set(listenFd_.get(), 0);
                acceptPaused_ = true;
            }
            return;
        }
        auto session = std::make_unique<Session>(FileDescriptor(fd), epoll_, dns_,
                                                 [this](Session* s) { release(s); });
        Session* raw = session.get();
        sessions_.emplace(raw, std::move(session));
        raw->start();
    }
}

void Server::release(Session* session) {
    auto it = sessions_.find(session);
    if (it == sessions_.end()) return;
    graveyard_.push_back(std::move(it->second));
    sessions_.erase(it);
    if (acceptPaused_) {
        acceptPaused_ = false;
        listenWatch_.set(listenFd_.get(), EPOLLIN);
    }
}