#pragma once

#include <chrono>
#include <string>

#include "interfaces.hpp"

class RetryingHttpClient final : public IHttpClient {
public:
    RetryingHttpClient(IHttpClient& inner, int maxRetries) : inner_(inner), maxRetries_(maxRetries) {}

    async::Future<HttpResponse> get(const std::string& url) override {
        return attempt(url, maxRetries_, 0, std::chrono::milliseconds(0));
    }

    async::Future<HttpResponse> getAfter(const std::string& url, std::chrono::milliseconds delay) override {
        return attempt(url, maxRetries_, 0, delay);
    }

private:
    async::Future<HttpResponse> attempt(const std::string& url, int retriesLeft, int attemptIndex,
                                        std::chrono::milliseconds delay);

    IHttpClient& inner_;
    int maxRetries_;
};