#pragma once

#include <netinet/in.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace socks5 {

constexpr uint8_t kVersion = 0x05;

constexpr uint8_t kAuthNone = 0x00;
constexpr uint8_t kAuthNoAcceptable = 0xFF;

constexpr uint8_t kCmdConnect = 0x01;

constexpr uint8_t kReserved = 0x00;

constexpr uint8_t kAtypIPv4 = 0x01;
constexpr uint8_t kAtypDomain = 0x03;
constexpr uint8_t kAtypIPv6 = 0x04;

constexpr uint8_t kRepSucceeded = 0x00;
constexpr uint8_t kRepGeneralFailure = 0x01;
constexpr uint8_t kRepNotAllowed = 0x02;
constexpr uint8_t kRepNetUnreachable = 0x03;
constexpr uint8_t kRepHostUnreachable = 0x04;
constexpr uint8_t kRepConnRefused = 0x05;
constexpr uint8_t kRepTtlExpired = 0x06;
constexpr uint8_t kRepCmdNotSupported = 0x07;
constexpr uint8_t kRepAtypNotSupported = 0x08;

constexpr size_t kGreetingMinSize = 2;
constexpr size_t kRequestMinSize = 4;
constexpr size_t kDomainLenOffset = 4;
constexpr size_t kIPv4AddrOffset = 4;
constexpr size_t kIPv4PortOffset = 8;
constexpr size_t kIPv4Size = 4;
constexpr size_t kPortSize = 2;
constexpr size_t kRequestFixedPrefix = 4;
constexpr size_t kDomainLenSize = 1;
constexpr size_t kMethodReplySize = 2;
constexpr size_t kReplySize = 10;
constexpr size_t kReplyAddrOffset = 4;
constexpr size_t kReplyPortOffset = 8;

enum class Status {
    NeedMore,
    Ok,
    Reject,
    Malformed,
};

struct Greeting {
    Status status = Status::NeedMore;
    bool noAuthOffered = false;
    size_t consumed = 0;
};

struct Request {
    Status status = Status::NeedMore;
    uint8_t replyCode = kRepSucceeded;
    size_t consumed = 0;
    bool isDomain = false;
    in_addr addr{};
    std::string host;
    uint16_t port = 0;
};

Greeting parseGreeting(const uint8_t* data, size_t size);
Request parseRequest(const uint8_t* data, size_t size);

std::array<uint8_t, kMethodReplySize> makeMethodReply(uint8_t method);
std::array<uint8_t, kReplySize> makeReply(uint8_t code, const sockaddr_in* bound = nullptr);

uint8_t replyForErrno(int err);

}