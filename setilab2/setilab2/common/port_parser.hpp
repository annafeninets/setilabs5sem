#pragma once

#include <cstdint>
#include <string>

inline bool parsePort(const std::string& text, uint16_t& port) {
    if (text.empty() || text.size() > 5) return false;
    unsigned value = 0;
    for (char c : text) {
        if (c < '0' || c > '9') return false;
        value = value * 10 + static_cast<unsigned>(c - '0');
    }
    if (value == 0 || value > 65535) return false;
    port = static_cast<uint16_t>(value);
    return true;
}
