#pragma once

#include <string>

#include "interfaces.hpp"

class RetryingHttpClient final : public IHttpClient {
public:
    RetryingHttpClient(IHttpClient& inner, int maxRetries) : inner_(inner), maxRetries_(maxRetries) {}

    async::Future<HttpResponse> get(const std::string& url) override { return attempt(url, maxRetries_); }

private:
    async::Future<HttpResponse> attempt(const std::string& url, int retriesLeft);

    IHttpClient& inner_;
    int maxRetries_;
};