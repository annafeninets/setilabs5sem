#include "dns.hpp"

#include <arpa/inet.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace dns {

bool buildQuery(uint16_t id, const std::string& host, std::vector<uint8_t>& out) {
    std::string h = host;
    if (!h.empty() && h.back() == '.') h.pop_back();
    if (h.empty() || h.size() > kMaxHostnameLen) return false;

    out.clear();
    auto put16 = [&](uint16_t v) {
        out.push_back(static_cast<uint8_t>(v >> kBitsPerByte));
        out.push_back(static_cast<uint8_t>(v & kByteMask));
    };
    put16(id);
    put16(kFlagsStandardQuery);
    put16(kQdCountOne);
    put16(kCountZero);
    put16(kCountZero);
    put16(kCountZero);

    size_t start = 0;
    for (;;) {
        size_t dot = h.find('.', start);
        if (dot == std::string::npos) dot = h.size();
        size_t len = dot - start;
        if (len == 0 || len > kMaxLabelLen) return false;
        out.push_back(static_cast<uint8_t>(len));
        out.insert(out.end(), h.begin() + start, h.begin() + dot);
        if (dot == h.size()) break;
        start = dot + 1;
    }
    out.push_back(kRootLabel);
    put16(kQTypeA);
    put16(kQClassIN);
    return true;
}

static long readName(const uint8_t* p, size_t n, size_t pos, std::string* out) {
    long next = -1;
    int jumps = 0;
    std::string name;
    for (;;) {
        if (pos >= n) return -1;
        uint8_t l = p[pos];
        if (l == kRootLabel) {
            ++pos;
            if (next < 0) next = static_cast<long>(pos);
            break;
        }
        if ((l & kCompressionMask) == kCompressionMask) {
            if (pos + 1 >= n) return -1;
            size_t ptr = (static_cast<size_t>(l & kCompressionPtrMask) << kBitsPerByte) | p[pos + 1];
            if (next < 0) next = static_cast<long>(pos + 2);
            if (++jumps > kMaxNameJumps) return -1;
            pos = ptr;
            continue;
        }
        if (l & kCompressionMask) return -1;
        ++pos;
        if (pos + l > n) return -1;
        if (out) {
            if (!name.empty()) name += '.';
            name.append(reinterpret_cast<const char*>(p + pos), l);
        }
        pos += l;
    }
    if (out) *out = std::move(name);
    return next;
}

bool parseResponse(const uint8_t* p, size_t n, Answer& out) {
    if (n < kHeaderSize) return false;
    auto get16 = [&](size_t pos) {
        return static_cast<uint16_t>((p[pos] << kBitsPerByte) | p[pos + 1]);
    };

    out = Answer{};
    out.id = get16(0);
    uint16_t flags = get16(2);
    if (!(flags & kFlagResponse)) return false;
    out.rcode = flags & kRcodeMask;
    uint16_t qd = get16(4), an = get16(6);

    size_t pos = kHeaderSize;
    for (uint16_t i = 0; i < qd; ++i) {
        std::string name;
        long next = readName(p, n, pos, &name);
        if (next < 0 || static_cast<size_t>(next) + kQuestionFixedSize > n) return false;
        if (i == 0) {
            std::transform(name.begin(), name.end(), name.begin(),
                           [](unsigned char c) { return std::tolower(c); });
            out.qname = name;
        }
        pos = static_cast<size_t>(next) + kQuestionFixedSize;
    }
    if (qd == 0) return false;

    for (uint16_t i = 0; i < an; ++i) {
        long next = readName(p, n, pos, nullptr);
        if (next < 0 || static_cast<size_t>(next) + kRrFixedSize > n) return false;
        pos = static_cast<size_t>(next);
        uint16_t type = get16(pos), cls = get16(pos + 2), rdlen = get16(pos + 8);
        pos += kRrFixedSize;
        if (pos + rdlen > n) return false;
        if (type == kQTypeA && cls == kQClassIN && rdlen == kARecordLen) {
            in_addr a;
            std::memcpy(&a, p + pos, kARecordLen);
            out.addrs.push_back(a);
        }
        pos += rdlen;
    }
    return true;
}

bool parseResolverAddress(const std::string& text, sockaddr_in& out) {
    std::string ip = text;
    long port = kDnsPort;
    size_t colon = text.find(':');
    if (colon != std::string::npos) {
        ip = text.substr(0, colon);
        std::string digits = text.substr(colon + 1);
        if (digits.empty() || digits.size() > kMaxPortDigits ||
            digits.find_first_not_of("0123456789") != std::string::npos) {
            return false;
        }
        port = std::strtol(digits.c_str(), nullptr, 10);
        if (port < kMinPort || port > kMaxPort) return false;
    }
    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip.c_str(), &sa.sin_addr) != 1) return false;
    sa.sin_port = htons(static_cast<uint16_t>(port));
    out = sa;
    return true;
}

sockaddr_in systemResolver() {
    sockaddr_in sa{};
    sa.sin_family = AF_INET;
    sa.sin_port = htons(kDnsPort);

    std::ifstream f("/etc/resolv.conf");
    std::string line;
    while (std::getline(f, line)) {
        std::istringstream ss(line);
        std::string key, val;
        if ((ss >> key >> val) && key == "nameserver" &&
            inet_pton(AF_INET, val.c_str(), &sa.sin_addr) == 1) {
            return sa;
        }
    }
    inet_pton(AF_INET, kDefaultResolver, &sa.sin_addr);
    return sa;
}
}