#include "resolver.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdio>
#include <random>
#include <system_error>

#include "dns.hpp"

namespace {
constexpr auto kTimeout = std::chrono::seconds(5);
constexpr size_t kMaxPending = 60000;
constexpr size_t kMaxAddrs = 8;
constexpr size_t kRecvBufSize = 4096;
constexpr int kMaxRecvPerEvent = 64;
constexpr int kRcodeNxDomain = 3;
constexpr int kTimerSlackMs = 1;
}

DnsResolver::DnsResolver(Epoll& epoll)
    : fd_(socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)),
      watch_(epoll, *this),
      server_(dns::systemResolver()),
      nextId_(static_cast<Id>(std::random_device{}())) {
    if (!fd_.valid()) throw std::system_error(errno, std::generic_category(), "socket(udp)");
    watch_.set(fd_.get(), EPOLLIN);

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_.sin_addr, ip, sizeof ip);
    std::fprintf(stderr, "DNS resolver %s\n", ip);
}

bool DnsResolver::resolve(const std::string& host, Callback callback, Id& id) {
    if (pending_.size() >= kMaxPending) return false;
    std::string name(host);
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    do {
        id = nextId_++;
    } while (pending_.count(id));

    std::vector<uint8_t> query;
    if (!dns::buildQuery(id, name, query)) return false;
    if (sendto(fd_.get(), query.data(), query.size(), 0, reinterpret_cast<sockaddr*>(&server_),
               sizeof server_) < 0) {
        return false;
    }
    std::fprintf(stderr, "resolve %s\n", name.c_str());
    auto deadline = Clock::now() + kTimeout;
    pending_[id] = Request{name, deadline, std::move(callback)};
    deadlines_.emplace_back(deadline, id);
    return true;
}

void DnsResolver::cancel(Id id) { pending_.erase(id); }

void DnsResolver::onEvent(uint32_t) {
    uint8_t buf[kRecvBufSize];
    for (int i = 0; i < kMaxRecvPerEvent; ++i) {
        sockaddr_in from{};
        socklen_t fl = sizeof from;
        ssize_t r = recvfrom(fd_.get(), buf, sizeof buf, 0, reinterpret_cast<sockaddr*>(&from), &fl);
        if (r < 0) return;
        if (from.sin_addr.s_addr != server_.sin_addr.s_addr || from.sin_port != server_.sin_port) continue;

        dns::Answer a;
        if (!dns::parseResponse(buf, static_cast<size_t>(r), a)) continue;
        auto it = pending_.find(a.id);
        if (it == pending_.end() || it->second.host != a.qname) continue;

        Callback cb = std::move(it->second.callback);
        pending_.erase(it);
        if (a.rcode != 0) {
            std::fprintf(stderr, "ERROR: resolve %s: DNS error (rcode %d%s)\n", a.qname.c_str(), a.rcode,
                         a.rcode == kRcodeNxDomain ? ", NXDOMAIN" : "");
            a.addrs.clear();
        } else if (a.addrs.empty()) {
            std::fprintf(stderr, "ERROR: resolve %s: no A records\n", a.qname.c_str());
        }
        if (a.addrs.size() > kMaxAddrs) a.addrs.resize(kMaxAddrs);
        cb(std::move(a.addrs));
    }
}

int DnsResolver::processTimeouts() {
    auto now = Clock::now();
    while (!deadlines_.empty()) {
        auto [deadline, id] = deadlines_.front();
        auto it = pending_.find(id);
        if (it == pending_.end() || it->second.deadline != deadline) {
            deadlines_.pop_front();
            continue;
        }
        if (deadline > now) {
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
            return static_cast<int>(ms) + kTimerSlackMs;
        }
        deadlines_.pop_front();
        std::fprintf(stderr, "ERROR: resolve %s: timeout\n", it->second.host.c_str());
        Callback cb = std::move(it->second.callback);
        pending_.erase(it);
        cb({});
    }
    return -1;
}