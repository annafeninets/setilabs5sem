#include "credentials.hpp"

#include <algorithm>

namespace {
    constexpr size_t kMaxFieldLen = 255;
    constexpr char kSeparator = ':';

    bool constantTimeEquals(const std::string& a, const std::string& b) {
        size_t n = std::max(a.size(), b.size());
        size_t diff = a.size() ^ b.size();
        for (size_t i = 0; i < n; ++i) {
            unsigned char x = i < a.size() ? static_cast<unsigned char>(a[i]) : 0;
            unsigned char y = i < b.size() ? static_cast<unsigned char>(b[i]) : 0;
            diff |= static_cast<size_t>(x ^ y);
        }
        return diff == 0;
    }
}

bool CredentialStore::add(const std::string& username, const std::string& password) {
    if (username.empty() || username.size() > kMaxFieldLen) return false;
    if (password.empty() || password.size() > kMaxFieldLen) return false;
    return users_.emplace(username, password).second;
}

bool CredentialStore::addFromString(const std::string& text) {
    size_t sep = text.find(kSeparator);
    if (sep == std::string::npos) return false;
    return add(text.substr(0, sep), text.substr(sep + 1));
}

bool CredentialStore::verify(const std::string& username, const std::string& password) const {
    static const std::string kNoPassword;
    auto it = users_.find(username);
    bool known = it != users_.end();
    bool match = constantTimeEquals(known ? it->second : kNoPassword, password);
    return known && match;
}