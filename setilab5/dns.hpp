#pragma once

#include <netinet/in.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace dns {

    constexpr size_t kMaxHostnameLen = 253;
    constexpr size_t kMaxLabelLen = 63;
    constexpr uint8_t kRootLabel = 0;
    constexpr uint8_t kByteMask = 0xFF;
    constexpr int kBitsPerByte = 8;

    constexpr uint16_t kFlagsStandardQuery = 0x0100;
    constexpr uint16_t kFlagResponse = 0x8000;
    constexpr uint16_t kRcodeMask = 0x0F;
    constexpr uint16_t kQdCountOne = 1;
    constexpr uint16_t kCountZero = 0;

    constexpr uint16_t kQTypeA = 1;
    constexpr uint16_t kQClassIN = 1;

    constexpr size_t kHeaderSize = 12;
    constexpr size_t kQuestionFixedSize = 4;
    constexpr size_t kRrFixedSize = 10;
    constexpr size_t kARecordLen = 4;

    constexpr uint8_t kCompressionMask = 0xC0;
    constexpr uint8_t kCompressionPtrMask = 0x3F;
    constexpr int kMaxNameJumps = 16;

    constexpr uint16_t kDnsPort = 53;
    constexpr long kMinPort = 1;
    constexpr long kMaxPort = 65535;
    constexpr size_t kMaxPortDigits = 5;
    constexpr const char* kDefaultResolver = "8.8.8.8";

    struct Answer {
        uint16_t id = 0;
        int rcode = 0;
        std::string qname;
        std::vector<in_addr> addrs;
    };

    bool buildQuery(uint16_t id, const std::string& host, std::vector<uint8_t>& out);

    bool parseResponse(const uint8_t* p, size_t n, Answer& out);

    bool parseResolverAddress(const std::string& text, sockaddr_in& out);

    sockaddr_in systemResolver();

}