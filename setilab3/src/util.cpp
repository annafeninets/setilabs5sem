#include "util.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace {

constexpr unsigned long kFirstCodePointNeedingTwoBytes = 0x80;
constexpr unsigned long kFirstCodePointNeedingThreeBytes = 0x800;
constexpr unsigned long kFirstCodePointNeedingFourBytes = 0x10000;

constexpr unsigned kLeadByteMarkerTwoBytes = 0xC0;
constexpr unsigned kLeadByteMarkerThreeBytes = 0xE0;
constexpr unsigned kLeadByteMarkerFourBytes = 0xF0;

constexpr unsigned kContinuationByteMarker = 0x80;
constexpr unsigned kContinuationPayloadMask = 0x3F;
constexpr unsigned kContinuationPayloadBits = 6;
constexpr unsigned kContinuationDetectMask = 0xC0;

constexpr char kHexDigits[] = "0123456789ABCDEF";
constexpr unsigned kBitsPerHexDigit = 4;
constexpr unsigned kLowHexDigitMask = 0x0F;

constexpr std::size_t kMaxEntityLength = 10;
constexpr int kDecimalRadix = 10;
constexpr int kHexadecimalRadix = 16;

constexpr const char* kLeftGuillemet = "\xC2\xAB";
constexpr const char* kRightGuillemet = "\xC2\xBB";
constexpr const char* kEnDash = "\xE2\x80\x93";
constexpr const char* kEmDash = "\xE2\x80\x94";
constexpr const char* kEllipsis = "\xE2\x80\xA6";

char continuationByte(unsigned long codePoint, unsigned groupIndex) {
    unsigned long group = (codePoint >> (kContinuationPayloadBits * groupIndex)) & kContinuationPayloadMask;
    return static_cast<char>(kContinuationByteMarker | group);
}

void appendUtf8(std::string& out, unsigned long codePoint) {
    if (codePoint < kFirstCodePointNeedingTwoBytes) {
        out += static_cast<char>(codePoint);
    } else if (codePoint < kFirstCodePointNeedingThreeBytes) {
        out += static_cast<char>(kLeadByteMarkerTwoBytes | (codePoint >> kContinuationPayloadBits));
        out += continuationByte(codePoint, 0);
    } else if (codePoint < kFirstCodePointNeedingFourBytes) {
        out += static_cast<char>(kLeadByteMarkerThreeBytes | (codePoint >> (2 * kContinuationPayloadBits)));
        out += continuationByte(codePoint, 1);
        out += continuationByte(codePoint, 0);
    } else {
        out += static_cast<char>(kLeadByteMarkerFourBytes | (codePoint >> (3 * kContinuationPayloadBits)));
        out += continuationByte(codePoint, 2);
        out += continuationByte(codePoint, 1);
        out += continuationByte(codePoint, 0);
    }
}

bool isContinuationByte(char byte) {
    return (static_cast<unsigned char>(byte) & kContinuationDetectMask) == kContinuationByteMarker;
}

}

std::string percentEncode(const std::string& s) {
    std::string out;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += static_cast<char>(c);
        } else {
            out += '%';
            out += kHexDigits[c >> kBitsPerHexDigit];
            out += kHexDigits[c & kLowHexDigitMask];
        }
    }
    return out;
}

std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

namespace {

std::string decodeEntities(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] != '&') {
            out += s[i];
            continue;
        }
        size_t semi = s.find(';', i);
        if (semi == std::string::npos || semi - i > kMaxEntityLength) {
            out += s[i];
            continue;
        }
        std::string ent = s.substr(i + 1, semi - i - 1);
        if (ent == "amp") out += '&';
        else if (ent == "lt") out += '<';
        else if (ent == "gt") out += '>';
        else if (ent == "quot") out += '"';
        else if (ent == "apos") out += '\'';
        else if (ent == "nbsp") out += ' ';
        else if (ent == "laquo") out += kLeftGuillemet;
        else if (ent == "raquo") out += kRightGuillemet;
        else if (ent == "ndash") out += kEnDash;
        else if (ent == "mdash") out += kEmDash;
        else if (ent == "hellip") out += kEllipsis;
        else if (!ent.empty() && ent[0] == '#') {
            try {
                unsigned long cp = (ent.size() > 1 && (ent[1] == 'x' || ent[1] == 'X'))
                                       ? std::stoul(ent.substr(2), nullptr, kHexadecimalRadix)
                                       : std::stoul(ent.substr(1), nullptr, kDecimalRadix);
                appendUtf8(out, cp);
            } catch (...) {
                out += s.substr(i, semi - i + 1);
            }
        } else {
            out += s.substr(i, semi - i + 1);
            i = semi;
            continue;
        }
        i = semi;
    }
    return out;
}

}

std::string stripHtml(const std::string& html) {
    std::string noTags;
    bool inTag = false;
    for (char c : html) {
        if (c == '<') inTag = true;
        else if (c == '>' && inTag) { inTag = false; noTags += ' '; }
        else if (!inTag) noTags += c;
    }
    std::string decoded = decodeEntities(noTags);
    std::string out;
    bool space = false;
    for (char c : decoded) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') { space = true; continue; }
        if (space && !out.empty()) out += ' ';
        space = false;
        out += c;
    }
    return out;
}

std::string truncateUtf8(const std::string& s, size_t maxChars) {
    size_t chars = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        if (!isContinuationByte(s[i])) {
            if (chars == maxChars) return s.substr(0, i) + "...";
            ++chars;
        }
    }
    return s;
}

std::string describeException(std::exception_ptr e) {
    try {
        std::rethrow_exception(std::move(e));
    } catch (const std::exception& ex) {
        return ex.what();
    } catch (...) {
        return "unknown error";
    }
}

std::map<std::string, std::string> loadEnv(const std::string& path) {
    std::map<std::string, std::string> result;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        result[trim(line.substr(0, eq))] = trim(line.substr(eq + 1));
    }
    return result;
}

std::string envOr(const std::map<std::string, std::string>& file, const std::string& key,
                  const std::string& fallback) {
    if (const char* v = std::getenv(key.c_str())) return v;
    auto it = file.find(key);
    return it != file.end() ? it->second : fallback;
}