#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "epoll.hpp"
#include "fd.hpp"
#include "resolver.hpp"
#include "session.hpp"

class Server : public EventHandler {
public:
    explicit Server(uint16_t port);

    [[noreturn]] void run();

    void onEvent(uint32_t events) override;

private:
    void release(Session* session);

    Epoll epoll_;
    FileDescriptor listenFd_;
    Watch listenWatch_;
    DnsResolver dns_;
    std::unordered_map<Session*, std::unique_ptr<Session>> sessions_;
    std::vector<std::unique_ptr<Session>> graveyard_;
    bool acceptPaused_ = false;
};