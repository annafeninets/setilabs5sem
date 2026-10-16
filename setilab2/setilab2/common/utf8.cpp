#include "utf8.hpp"

#include <cstdint>

namespace {

constexpr unsigned char kTwoByteLeadMask = 0xE0;
constexpr unsigned char kTwoByteLeadPattern = 0xC0;
constexpr unsigned char kTwoByteLeadPayloadMask = 0x1F;

constexpr unsigned char kThreeByteLeadMask = 0xF0;
constexpr unsigned char kThreeByteLeadPattern = 0xE0;
constexpr unsigned char kThreeByteLeadPayloadMask = 0x0F;

constexpr unsigned char kFourByteLeadMask = 0xF8;
constexpr unsigned char kFourByteLeadPattern = 0xF0;
constexpr unsigned char kFourByteLeadPayloadMask = 0x07;

constexpr unsigned char kContinuationMask = 0xC0;
constexpr unsigned char kContinuationPattern = 0x80;
constexpr unsigned char kContinuationPayloadMask = 0x3F;
constexpr int kContinuationPayloadBits = 6;

constexpr unsigned char kAsciiLimit = 0x80;

constexpr uint32_t kMinCodepointTwoByte = 0x80;
constexpr uint32_t kMinCodepointThreeByte = 0x800;
constexpr uint32_t kMinCodepointFourByte = 0x10000;

constexpr uint32_t kSurrogateRangeStart = 0xD800;
constexpr uint32_t kSurrogateRangeEnd = 0xDFFF;
constexpr uint32_t kMaxUnicodeCodepoint = 0x10FFFF;

size_t decodeOne(const unsigned char* bytes, size_t len, size_t i) {
    unsigned char b0 = bytes[i];

    if (b0 < kAsciiLimit) {
        return 1;
    }

    size_t extra;
    uint32_t codepoint;
    uint32_t minCodepoint;

    if ((b0 & kTwoByteLeadMask) == kTwoByteLeadPattern) {
        extra = 1;
        codepoint = b0 & kTwoByteLeadPayloadMask;
        minCodepoint = kMinCodepointTwoByte;
    } else if ((b0 & kThreeByteLeadMask) == kThreeByteLeadPattern) {
        extra = 2;
        codepoint = b0 & kThreeByteLeadPayloadMask;
        minCodepoint = kMinCodepointThreeByte;
    } else if ((b0 & kFourByteLeadMask) == kFourByteLeadPattern) {
        extra = 3;
        codepoint = b0 & kFourByteLeadPayloadMask;
        minCodepoint = kMinCodepointFourByte;
    } else {
        return 0;
    }

    if (i + extra >= len) {
        return 0;
    }

    for (size_t k = 1; k <= extra; ++k) {
        unsigned char continuationByte = bytes[i + k];
        if ((continuationByte & kContinuationMask) != kContinuationPattern) {
            return 0;
        }
        codepoint = (codepoint << kContinuationPayloadBits) |
                    (continuationByte & kContinuationPayloadMask);
    }

    if (codepoint < minCodepoint) {
        return 0;
    }
    if (codepoint >= kSurrogateRangeStart && codepoint <= kSurrogateRangeEnd) {
        return 0;
    }
    if (codepoint > kMaxUnicodeCodepoint) {
        return 0;
    }

    return extra + 1;
}

}

bool isValidUtf8(const std::string& s) noexcept {
    const auto* bytes = reinterpret_cast<const unsigned char*>(s.data());
    const size_t len = s.size();

    size_t i = 0;
    while (i < len) {
        size_t charLen = decodeOne(bytes, len, i);
        if (charLen == 0) {
            return false;
        }
        i += charLen;
    }
    return true;
}
