#pragma once

#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "future.hpp"
#include "interfaces.hpp"

class HttpClient final : public IHttpClient {
public:
    HttpClient();
    ~HttpClient() override;
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    async::Future<HttpResponse> get(const std::string& url) override;

private:
    struct Request;
    void run();

    void* multi_ = nullptr;
    std::mutex mutex_;
    std::deque<std::unique_ptr<Request>> pending_;
    std::atomic<bool> stop_{false};
    std::thread thread_;
};
