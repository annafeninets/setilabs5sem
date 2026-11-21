#pragma once

#include <netinet/in.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "credentials.hpp"
#include "epoll.hpp"
#include "fd.hpp"
#include "resolver.hpp"
#include "session.hpp"

class Server : public EventHandler {
public:
    Server(uint16_t port, const sockaddr_in& dnsServer, CredentialStore credentials);

    [[noreturn]] void run();

    void onEvent(uint32_t events) override;

private:
    using Clock = std::chrono::steady_clock;

    void release(Session* session);
    void pauseAccept();
    void resumeAccept();
    int msUntilAcceptResume() const;

    Epoll epoll_;
    FileDescriptor listenFd_;
    Watch listenWatch_;
    DnsResolver dns_;
    CredentialStore credentials_;
    std::unordered_map<Session*, std::unique_ptr<Session>> sessions_;
    std::vector<std::unique_ptr<Session>> graveyard_;
    bool acceptPaused_ = false;
    Clock::time_point acceptResumeAt_;
};