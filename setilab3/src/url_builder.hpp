#pragma once

#include <string>
#include <utility>
#include <vector>

class UrlBuilder {
public:
    explicit UrlBuilder(std::string base) : base_(std::move(base)) {}

    UrlBuilder& param(const std::string& key, const std::string& value);
    UrlBuilder& param(const std::string& key, double value);
    UrlBuilder& path(const std::string& segment);
    std::string build() const;

private:
    std::string base_;
    std::vector<std::pair<std::string, std::string>> params_;
};
