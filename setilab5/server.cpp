#include "server.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <system_error>

namespace {
constexpr auto kAcceptRetryDelay = std::chrono::seconds(1);
constexpr int kTimerSlackMs = 1;

[[noreturn]] void fail(const char* what) {
    throw std::system_error(errno, std::generic_category(), what);
}
}

Server::Server(uint16_t port, const sockaddr_in& dnsServer)
    : listenFd_(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)),
      listenWatch_(epoll_, *this),
      dns_(epoll_, dnsServer) {
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
        if (acceptPaused_ && Clock::now() >= acceptResumeAt_) resumeAccept();
        int timeout = dns_.processTimeouts();
        if (acceptPaused_) {
            int wait = msUntilAcceptResume();
            if (timeout < 0 || wait < timeout) timeout = wait;
        }
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
            pauseAccept();
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
    if (acceptPaused_) resumeAccept();
}

void Server::pauseAccept() {
    listenWatch_.set(listenFd_.get(), 0);
    acceptPaused_ = true;
    acceptResumeAt_ = Clock::now() + kAcceptRetryDelay;
}

void Server::resumeAccept() {
    acceptPaused_ = false;
    listenWatch_.set(listenFd_.get(), EPOLLIN);
}

int Server::msUntilAcceptResume() const {
    auto left = std::chrono::duration_cast<std::chrono::milliseconds>(acceptResumeAt_ - Clock::now()).count();
    return static_cast<int>(left < 0 ? 0 : left) + kTimerSlackMs;
}