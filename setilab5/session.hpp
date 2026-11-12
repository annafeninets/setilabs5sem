#pragma once

#include <netinet/in.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "buffer.hpp"
#include "epoll.hpp"
#include "fd.hpp"
#include "resolver.hpp"

class Session {
public:
    using ClosedCallback = std::function<void(Session*)>;

    Session(FileDescriptor client, Epoll& epoll, DnsResolver& dns, ClosedCallback onClosed);
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    void start();
    bool closed() const { return closed_; }

private:
    enum class State {
        Greeting,
        Request,
        Resolving,
        Connecting,
        Relay,
        Failing,
    };

    class Side : public EventHandler {
    public:
        Side(Session& s, bool client) : session_(s), client_(client) {}
        void onEvent(uint32_t events) override {
            if (client_) session_.onClientEvent(events);
            else session_.onRemoteEvent(events);
        }

    private:
        Session& session_;
        bool client_;
    };

    void onClientEvent(uint32_t events);
    void onRemoteEvent(uint32_t events);

    void readClient();
    void readRemote();
    bool flush(int fd, Buffer& buf);

    void parseHandshake();
    void startResolve();
    void onResolved(std::vector<in_addr> addrs);
    void connectNext(int lastErr);
    void onConnectResult();
    void onConnected();

    void fail(uint8_t replyCode, const std::string& reason);
    void log(const char* fmt, ...) const __attribute__((format(printf, 2, 3)));
    void progress();
    void updateInterest();
    void closeRemote();
    void close();

    FileDescriptor client_;
    FileDescriptor remote_;
    Epoll& epoll_;
    DnsResolver& dns_;
    ClosedCallback onClosed_;
    Side clientSide_, remoteSide_;
    Watch clientWatch_, remoteWatch_;

    State state_ = State::Greeting;
    bool closed_ = false;

    Buffer c2r_;
    Buffer r2c_;
    bool clientEof_ = false;
    bool remoteEof_ = false;
    bool remoteWrShut_ = false;
    bool clientWrShut_ = false;

    std::string peer_;
    std::string target_;
    std::string host_;
    uint16_t port_ = 0;
    std::vector<in_addr> addrs_;
    size_t nextAddr_ = 0;

    bool dnsPending_ = false;
    DnsResolver::Id dnsId_ = 0;
};