#pragma once

#include <netinet/in.h>

#include <chrono>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "epoll.hpp"
#include "fd.hpp"

class DnsResolver : public EventHandler {
public:
    using Callback = std::function<void(std::vector<in_addr>)>;
    using Id = uint16_t;

    explicit DnsResolver(Epoll& epoll);

    bool resolve(const std::string& host, Callback callback, Id& id);
    void cancel(Id id);

    int processTimeouts();

    void onEvent(uint32_t events) override;

private:
    using Clock = std::chrono::steady_clock;

    struct Request {
        std::string host;
        Clock::time_point deadline;
        Callback callback;
    };

    FileDescriptor fd_;
    Watch watch_;
    sockaddr_in server_{};
    Id nextId_;
    std::unordered_map<Id, Request> pending_;
    std::deque<std::pair<Clock::time_point, Id>> deadlines_;
};