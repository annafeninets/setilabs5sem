#include "socks5.hpp"

#include <cerrno>
#include <cstring>

namespace socks5 {

Greeting parseGreeting(const uint8_t* p, size_t size) {
    Greeting g;
    if (size < kGreetingMinSize) return g;
    if (p[0] != kVersion) {
        g.status = Status::Malformed;
        return g;
    }
    size_t nmethods = p[1];
    if (size < kGreetingMinSize + nmethods) return g;
    for (size_t i = 0; i < nmethods; ++i) g.noAuthOffered |= (p[kGreetingMinSize + i] == kAuthNone);
    g.consumed = kGreetingMinSize + nmethods;
    g.status = Status::Ok;
    return g;
}

Request parseRequest(const uint8_t* p, size_t size) {
    Request r;
    if (size < kRequestMinSize) return r;
    if (p[0] != kVersion || p[2] != kReserved) {
        r.status = Status::Malformed;
        return r;
    }
    if (p[1] != kCmdConnect) {
        r.status = Status::Reject;
        r.replyCode = kRepCmdNotSupported;
        return r;
    }

    uint8_t atyp = p[3];
    size_t need;
    switch (atyp) {
        case kAtypIPv4:
            need = kRequestFixedPrefix + kIPv4Size + kPortSize;
            break;
        case kAtypDomain:
            if (size < kDomainLenOffset + kDomainLenSize) return r;
            need = kRequestFixedPrefix + kDomainLenSize + p[kDomainLenOffset] + kPortSize;
            break;
        default:
            r.status = Status::Reject;
            r.replyCode = kRepAtypNotSupported;
            return r;
    }
    if (size < need) return r;

    if (atyp == kAtypIPv4) {
        std::memcpy(&r.addr, p + kIPv4AddrOffset, kIPv4Size);
        r.port = static_cast<uint16_t>((p[kIPv4PortOffset] << 8) | p[kIPv4PortOffset + 1]);
    } else {
        size_t len = p[kDomainLenOffset];
        r.isDomain = true;
        r.host.assign(reinterpret_cast<const char*>(p + kDomainLenOffset + kDomainLenSize), len);
        r.port = static_cast<uint16_t>((p[kDomainLenOffset + kDomainLenSize + len] << 8) |
                                      p[kDomainLenOffset + kDomainLenSize + len + 1]);
    }
    r.consumed = need;
    r.status = Status::Ok;
    return r;
}

std::array<uint8_t, kMethodReplySize> makeMethodReply(uint8_t method) {
    return {kVersion, method};
}

std::array<uint8_t, kReplySize> makeReply(uint8_t code, const sockaddr_in* bound) {
    std::array<uint8_t, kReplySize> m = {kVersion, code, kReserved, kAtypIPv4, 0, 0, 0, 0, 0, 0};
    if (bound) {
        std::memcpy(m.data() + kReplyAddrOffset, &bound->sin_addr, kIPv4Size);
        std::memcpy(m.data() + kReplyPortOffset, &bound->sin_port, kPortSize);
    }
    return m;
}

uint8_t replyForErrno(int err) {
    switch (err) {
        case ECONNREFUSED:
            return kRepConnRefused;
        case ENETUNREACH:
            return kRepNetUnreachable;
        case EHOSTUNREACH:
            return kRepHostUnreachable;
        case ETIMEDOUT:
            return kRepTtlExpired;
        case EACCES:
        case EPERM:
            return kRepNotAllowed;
        default:
            return kRepGeneralFailure;
    }
}
}