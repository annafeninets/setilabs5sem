#include "session.hpp"

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "socks5.hpp"

namespace {
constexpr size_t kBufSize = 64 * 1024;
constexpr size_t kLogBufSize = 512;

void setNoDelay(int fd) {
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
}
}

Session::Session(FileDescriptor client, Epoll& epoll, DnsResolver& dns, ClosedCallback onClosed)
    : client_(std::move(client)),
      epoll_(epoll),
      dns_(dns),
      onClosed_(std::move(onClosed)),
      clientSide_(*this, true),
      remoteSide_(*this, false),
      clientWatch_(epoll, clientSide_),
      remoteWatch_(epoll, remoteSide_),
      c2r_(kBufSize),
      r2c_(kBufSize) {}

void Session::log(const char* fmt, ...) const {
    char msg[kLogBufSize];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    std::fprintf(stderr, "[%s] %s\n", peer_.c_str(), msg);
}

void Session::start() {
    sockaddr_in pa{};
    socklen_t pl = sizeof pa;
    char ip[INET_ADDRSTRLEN] = "?";
    if (getpeername(client_.get(), reinterpret_cast<sockaddr*>(&pa), &pl) == 0)
        inet_ntop(AF_INET, &pa.sin_addr, ip, sizeof ip);
    peer_ = std::string(ip) + ":" + std::to_string(ntohs(pa.sin_port));
    setNoDelay(client_.get());
    clientWatch_.set(client_.get(), EPOLLIN);
}

void Session::closeRemote() {
    remoteWatch_.remove();
    remote_.reset();
}

void Session::close() {
    if (closed_) return;
    closed_ = true;
    if (dnsPending_) dns_.cancel(dnsId_);
    dnsPending_ = false;
    clientWatch_.remove();
    client_.reset();
    closeRemote();
    onClosed_(this);
}

void Session::fail(uint8_t replyCode, const std::string& reason) {
    log("ERROR: %s (reply 0x%02X)", reason.c_str(), replyCode);
    closeRemote();
    if (dnsPending_) dns_.cancel(dnsId_);
    dnsPending_ = false;
    c2r_.clear();
    r2c_.clear();
    auto msg = socks5::makeReply(replyCode);
    r2c_.append(msg.data(), msg.size());
    state_ = State::Failing;
    progress();
}

void Session::onClientEvent(uint32_t ev) {
    if (closed_) return;
    if (ev & EPOLLERR) {
        log("ERROR: client socket error");
        return close();
    }
    if (ev & EPOLLIN) {
        readClient();
        if (closed_) return;
    }
    if ((ev & EPOLLHUP) && !(ev & EPOLLIN)) return close();
    progress();
}

void Session::onRemoteEvent(uint32_t ev) {
    if (closed_) return;
    if (state_ == State::Connecting) return onConnectResult();
    if (ev & EPOLLERR) {
        log("ERROR: connection to %s: socket error", target_.c_str());
        return close();
    }
    if (ev & EPOLLIN) {
        readRemote();
        if (closed_) return;
    }
    if ((ev & EPOLLHUP) && !(ev & EPOLLIN)) return close();
    progress();
}

void Session::readClient() {
    if (clientEof_ || c2r_.space() == 0) return;
    size_t n;
    uint8_t* p = c2r_.writePtr(n);
    ssize_t r = recv(client_.get(), p, n, 0);
    if (r > 0) {
        c2r_.commit(static_cast<size_t>(r));
        if (state_ == State::Greeting || state_ == State::Request) parseHandshake();
    } else if (r == 0) {
        clientEof_ = true;
        if (state_ != State::Relay) {
            log("client disconnected before the connection was established");
            close();
        }
    } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        log("ERROR: reading from client: %s", std::strerror(errno));
        close();
    }
}

void Session::readRemote() {
    if (state_ != State::Relay || remoteEof_ || r2c_.space() == 0) return;
    size_t n;
    uint8_t* p = r2c_.writePtr(n);
    ssize_t r = recv(remote_.get(), p, n, 0);
    if (r > 0) {
        r2c_.commit(static_cast<size_t>(r));
    } else if (r == 0) {
        remoteEof_ = true;
    } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        log("ERROR: reading from %s: %s", target_.c_str(), std::strerror(errno));
        close();
    }
}

void Session::parseHandshake() {
    if (state_ == State::Greeting) {
        auto g = socks5::parseGreeting(c2r_.readPtr(), c2r_.size());
        if (g.status == socks5::Status::NeedMore) return;
        if (g.status == socks5::Status::Malformed) {
            log("ERROR: protocol violation in greeting (not SOCKS5?)");
            return close();
        }
        c2r_.consume(g.consumed);
        if (!g.noAuthOffered) {
            log("ERROR: client offers no acceptable authentication method (only no-auth is supported)");
            c2r_.clear();
            r2c_.clear();
            auto msg = socks5::makeMethodReply(socks5::kAuthNoAcceptable);
            r2c_.append(msg.data(), msg.size());
            state_ = State::Failing;
            return;
        }
        auto msg = socks5::makeMethodReply(socks5::kAuthNone);
        r2c_.append(msg.data(), msg.size());
        state_ = State::Request;
    }

    if (state_ == State::Request) {
        auto r = socks5::parseRequest(c2r_.readPtr(), c2r_.size());
        if (r.status == socks5::Status::NeedMore) return;
        if (r.status == socks5::Status::Malformed) {
            log("ERROR: protocol violation in request");
            return close();
        }
        if (r.status == socks5::Status::Reject) {
            return fail(r.replyCode, r.replyCode == socks5::kRepCmdNotSupported
                                         ? "unsupported command (only CONNECT is supported)"
                                         : "unsupported address type (only IPv4 and domain names)");
        }

        c2r_.consume(r.consumed);
        port_ = r.port;
        if (r.isDomain) {
            host_ = r.host;
            for (char& ch : host_) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            target_ = host_ + ":" + std::to_string(port_);
            startResolve();
        } else {
            addrs_.assign(1, r.addr);
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &r.addr, ip, sizeof ip);
            target_ = std::string(ip) + ":" + std::to_string(port_);
            nextAddr_ = 0;
            connectNext(ECONNREFUSED);
        }
    }
}

void Session::startResolve() {
    bool sent = dns_.resolve(
        host_, [this](std::vector<in_addr> addrs) { onResolved(std::move(addrs)); }, dnsId_);
    if (!sent) return fail(socks5::kRepHostUnreachable, "cannot send DNS query for " + host_);
    dnsPending_ = true;
    state_ = State::Resolving;
}

void Session::onResolved(std::vector<in_addr> addrs) {
    dnsPending_ = false;
    if (addrs.empty()) return fail(socks5::kRepHostUnreachable, "cannot resolve " + host_);
    addrs_ = std::move(addrs);
    nextAddr_ = 0;
    connectNext(EHOSTUNREACH);
}

void Session::connectNext(int lastErr) {
    while (nextAddr_ < addrs_.size()) {
        in_addr a = addrs_[nextAddr_++];
        FileDescriptor fd(socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0));
        if (!fd.valid()) {
            lastErr = errno;
            break;
        }
        sockaddr_in sa{};
        sa.sin_family = AF_INET;
        sa.sin_addr = a;
        sa.sin_port = htons(port_);
        int r = connect(fd.get(), reinterpret_cast<sockaddr*>(&sa), sizeof sa);
        if (r < 0 && errno != EINPROGRESS) {
            lastErr = errno;
            continue;
        }
        setNoDelay(fd.get());
        remote_ = std::move(fd);
        state_ = State::Connecting;
        if (r == 0) onConnected();
        else updateInterest();
        return;
    }
    fail(socks5::replyForErrno(lastErr),
         "cannot connect to " + target_ + ": " + std::strerror(lastErr));
}

void Session::onConnectResult() {
    int err = 0;
    socklen_t l = sizeof err;
    getsockopt(remote_.get(), SOL_SOCKET, SO_ERROR, &err, &l);
    if (err != 0) {
        closeRemote();
        connectNext(err);
        return;
    }
    onConnected();
}

void Session::onConnected() {
    sockaddr_in bound{};
    socklen_t l = sizeof bound;
    getsockname(remote_.get(), reinterpret_cast<sockaddr*>(&bound), &l);
    auto msg = socks5::makeReply(socks5::kRepSucceeded, &bound);
    r2c_.append(msg.data(), msg.size());
    state_ = State::Relay;

    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addrs_[nextAddr_ - 1], ip, sizeof ip);
    log("connected to %s (%s:%u)", target_.c_str(), ip, port_);
    progress();
}

bool Session::flush(int fd, Buffer& buf) {
    ssize_t r = send(fd, buf.readPtr(), buf.size(), MSG_NOSIGNAL);
    if (r > 0) {
        buf.consume(static_cast<size_t>(r));
        return true;
    }
    if (r < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)) return true;
    log("ERROR: sending data: %s", std::strerror(errno));
    close();
    return false;
}

void Session::progress() {
    if (closed_) return;
    if (!r2c_.empty() && !flush(client_.get(), r2c_)) return;
    if (state_ == State::Relay && !c2r_.empty() && !flush(remote_.get(), c2r_)) return;

    if (state_ == State::Failing) {
        if (r2c_.empty()) close();
        else updateInterest();
        return;
    }
    if (state_ == State::Relay) {
        if (clientEof_ && c2r_.empty() && !remoteWrShut_) {
            shutdown(remote_.get(), SHUT_WR);
            remoteWrShut_ = true;
        }
        if (remoteEof_ && r2c_.empty() && !clientWrShut_) {
            shutdown(client_.get(), SHUT_WR);
            clientWrShut_ = true;
        }
        if (remoteWrShut_ && clientWrShut_) return close();
    }
    updateInterest();
}

void Session::updateInterest() {
    uint32_t cm = 0, rm = 0;
    switch (state_) {
        case State::Failing:
            break;
        case State::Greeting:
        case State::Request:
        case State::Resolving:
            if (!clientEof_ && c2r_.space() > 0) cm |= EPOLLIN;
            break;
        case State::Connecting:
            if (!clientEof_ && c2r_.space() > 0) cm |= EPOLLIN;
            rm |= EPOLLOUT;
            break;
        case State::Relay:
            if (!clientEof_ && c2r_.space() > 0) cm |= EPOLLIN;
            if (!remoteEof_ && r2c_.space() > 0) rm |= EPOLLIN;
            if (!c2r_.empty()) rm |= EPOLLOUT;
            break;
    }
    if (!r2c_.empty()) cm |= EPOLLOUT;

    clientWatch_.set(client_.get(), cm);
    if (remote_.valid()) remoteWatch_.set(remote_.get(), rm);
}