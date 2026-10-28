#include "url_builder.hpp"

#include <cstdio>

#include "util.hpp"

UrlBuilder& UrlBuilder::param(const std::string& key, const std::string& value) {
    params_.emplace_back(key, value);
    return *this;
}

UrlBuilder& UrlBuilder::param(const std::string& key, double value) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.6f", value);
    return param(key, std::string(buf));
}

UrlBuilder& UrlBuilder::path(const std::string& segment) {
    base_ += "/" + percentEncode(segment);
    return *this;
}

std::string UrlBuilder::build() const {
    std::string url = base_;
    char sep = '?';
    for (const auto& [key, value] : params_) {
        url += sep;
        url += percentEncode(key) + "=" + percentEncode(value);
        sep = '&';
    }
    return url;
}
