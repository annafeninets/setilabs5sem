#include "http_client.hpp"

#include <curl/curl.h>

#include "errors.hpp"
#include <unordered_map>

struct HttpClient::Request {
    CURL* easy = nullptr;
    std::string body;
    std::string host;
    char error[CURL_ERROR_SIZE] = {0};
    async::Promise<HttpResponse> promise;
};

namespace {

std::string hostOf(const std::string& url) {
    const std::string scheme = "://";
    std::size_t start = url.find(scheme);
    start = (start == std::string::npos) ? 0 : start + scheme.size();
    std::size_t end = url.find_first_of("/?:", start);
    return url.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

size_t writeCallback(char* data, size_t size, size_t count, void* userdata) {
    auto* body = static_cast<std::string*>(userdata);
    body->append(data, size * count);
    return size * count;
}

struct CurlGlobal {
    CurlGlobal() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobal() { curl_global_cleanup(); }
};

}

HttpClient::HttpClient() {
    static CurlGlobal global;
    multi_ = curl_multi_init();
    if (!multi_) throw NetworkError("curl_multi_init failed");
    thread_ = std::thread([this] { run(); });
}

HttpClient::~HttpClient() {
    stop_ = true;
    curl_multi_wakeup(static_cast<CURLM*>(multi_));
    if (thread_.joinable()) thread_.join();
    curl_multi_cleanup(static_cast<CURLM*>(multi_));
}

async::Future<HttpResponse> HttpClient::get(const std::string& url) {
    auto req = std::make_unique<Request>();
    req->host = hostOf(url);
    req->easy = curl_easy_init();
    if (!req->easy) {
        req->promise.setError(std::make_exception_ptr(NetworkError("curl_easy_init failed")));
        return req->promise.future();
    }
    curl_easy_setopt(req->easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(req->easy, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(req->easy, CURLOPT_WRITEDATA, &req->body);
    curl_easy_setopt(req->easy, CURLOPT_ERRORBUFFER, req->error);
    curl_easy_setopt(req->easy, CURLOPT_TIMEOUT, 20L);
    curl_easy_setopt(req->easy, CURLOPT_CONNECTTIMEOUT, 20L);
    curl_easy_setopt(req->easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(req->easy, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(req->easy, CURLOPT_USERAGENT, "lab3-places-cpp/1.0");
    curl_easy_setopt(req->easy, CURLOPT_NOSIGNAL, 1L);

    auto future = req->promise.future();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.push_back(std::move(req));
    }
    curl_multi_wakeup(static_cast<CURLM*>(multi_));
    return future;
}

void HttpClient::run() {
    CURLM* multi = static_cast<CURLM*>(multi_);
    std::unordered_map<CURL*, std::unique_ptr<Request>> active;

    auto fail = [](std::unique_ptr<Request>& r, const char* message) {
        curl_easy_cleanup(r->easy);
        r->promise.setError(std::make_exception_ptr(NetworkError(message)));
    };

    while (true) {
        std::deque<std::unique_ptr<Request>> incoming;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            incoming.swap(pending_);
        }

        if (stop_) {
            for (auto& r : incoming) fail(r, "HTTP client stopped");
            for (auto& [easy, r] : active) {
                curl_multi_remove_handle(multi, easy);
                fail(r, "HTTP client stopped");
            }
            return;
        }

        for (auto& r : incoming) {
            CURL* easy = r->easy;
            curl_multi_add_handle(multi, easy);
            active.emplace(easy, std::move(r));
        }

        int running = 0;
        curl_multi_perform(multi, &running);

        int left = 0;
        while (CURLMsg* msg = curl_multi_info_read(multi, &left)) {
            if (msg->msg != CURLMSG_DONE) continue;
            CURL* easy = msg->easy_handle;
            CURLcode code = msg->data.result;
            auto it = active.find(easy);
            if (it == active.end()) continue;
            std::unique_ptr<Request> req = std::move(it->second);
            active.erase(it);
            curl_multi_remove_handle(multi, easy);

            long status = 0;
            curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &status);
            std::string message = req->error[0] ? req->error : curl_easy_strerror(code);
            curl_easy_cleanup(easy);

            if (code == CURLE_OK) {
                req->promise.setValue(HttpResponse(status, std::move(req->body)));
            } else {
                req->promise.setError(std::make_exception_ptr(NetworkError("network error (" + req->host + "): " + message)));
            }
        }

        curl_multi_poll(multi, nullptr, 0, 1000, nullptr);
    }
}