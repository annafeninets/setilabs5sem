#pragma once

#include <string>
#include <unordered_map>

class CredentialStore {
public:
    bool add(const std::string& username, const std::string& password);
    bool addFromString(const std::string& text);

    bool empty() const { return users_.empty(); }
    size_t size() const { return users_.size(); }

    bool verify(const std::string& username, const std::string& password) const;

private:
    std::unordered_map<std::string, std::string> users_;
};