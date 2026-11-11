#include "http_client.hpp"

#include <curl/curl.h>

#include <algorithm>
#include <unordered_map>

#include "errors.hpp"

namespace {

constexpr std::size_t kBytesPerMebibyte = 1024 * 1024;
constexpr std::size_t kMaxResponseMebibytes = 5;
constexpr std::size_t kMaxResponseBytes = kMaxResponseMebibytes * kBytesPerMebibyte;
constexpr long kRequestTimeoutSeconds = 20;
constexpr long kConnectTimeoutSeconds = 20;
constexpr std::chrono::milliseconds kMaxPollWait{1000};

struct ResponseBuffer {
    std::string body;
    bool tooLarge = false;
};

std::string hostOf(const std::string& url) {
    const std::string scheme = "://";
    std::size_t start = url.find(scheme);
    start = (start == std::string::npos) ? 0 : start + scheme.size();
    std::size_t end = url.find_first_of("/?:", start);
    return url.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

size_t writeCallback(char* data, size_t size, size_t count, void* userdata) {
    auto* buffer = static_cast<ResponseBuffer*>(userdata);
    size_t chunk = size * count;
    if (buffer->body.size() + chunk > kMaxResponseBytes) {
        buffer->tooLarge = true;
        return 0;
    }
    buffer->body.append(data, chunk);
    return chunk;
}

struct CurlGlobal {
    CurlGlobal() { curl_global_init(CURL_GLOBAL_DEFAULT); }
    ~CurlGlobal() { curl_global_cleanup(); }
};

}

struct HttpClient::Request {
    CURL* easy = nullptr;
    ResponseBuffer buffer;
    std::string host;
    std::chrono::steady_clock::time_point notBefore;
    char error[CURL_ERROR_SIZE] = {0};
    async::Promise<HttpResponse> promise;
};

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
    return getAfter(url, std::chrono::milliseconds(0));
}

async::Future<HttpResponse> HttpClient::getAfter(const std::string& url, std::chrono::milliseconds delay) {
    auto req = std::make_unique<Request>();
    req->host = hostOf(url);
    req->notBefore = std::chrono::steady_clock::now() + delay;
    req->easy = curl_easy_init();
    if (!req->easy) {
        req->promise.setError(std::make_exception_ptr(NetworkError("curl_easy_init failed")));
        return req->promise.future();
    }
    curl_easy_setopt(req->easy, CURLOPT_URL, url.c_str());
    curl_easy_setopt(req->easy, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(req->easy, CURLOPT_WRITEDATA, &req->buffer);
    curl_easy_setopt(req->easy, CURLOPT_ERRORBUFFER, req->error);
    curl_easy_setopt(req->easy, CURLOPT_MAXFILESIZE_LARGE, static_cast<curl_off_t>(kMaxResponseBytes));
    curl_easy_setopt(req->easy, CURLOPT_TIMEOUT, kRequestTimeoutSeconds);
    curl_easy_setopt(req->easy, CURLOPT_CONNECTTIMEOUT, kConnectTimeoutSeconds);
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
    std::vector<std::unique_ptr<Request>> waiting;

    auto fail = [](std::unique_ptr<Request>& r, const std::string& message) {
        curl_easy_cleanup(r->easy);
        r->promise.setError(std::make_exception_ptr(NetworkError(message)));
    };

    auto failAllActive = [&](const std::string& message) {
        auto doomed = std::move(active);
        active.clear();
        for (auto& [easy, r] : doomed) {
            curl_multi_remove_handle(multi, easy);
            fail(r, "network error (" + r->host + "): " + message);
        }
    };

    while (true) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& r : pending_) waiting.push_back(std::move(r));
            pending_.clear();
        }

        if (stop_) {
            for (auto& r : waiting) fail(r, "HTTP client stopped");
            for (auto& [easy, r] : active) {
                curl_multi_remove_handle(multi, easy);
                fail(r, "HTTP client stopped");
            }
            return;
        }

        auto now = std::chrono::steady_clock::now();
        auto nextWake = now + kMaxPollWait;
        for (auto it = waiting.begin(); it != waiting.end();) {
            if ((*it)->notBefore <= now) {
                CURL* easy = (*it)->easy;
                CURLMcode added = curl_multi_add_handle(multi, easy);
                if (added == CURLM_OK) {
                    active.emplace(easy, std::move(*it));
                } else {
                    fail(*it, "network error (" + (*it)->host + "): " + curl_multi_strerror(added));
                }
                it = waiting.erase(it);
            } else {
                nextWake = min(nextWake, (*it)->notBefore);
                ++it;
            }
        }

        int running = 0;
        CURLMcode performed = curl_multi_perform(multi, &running);
        if (performed != CURLM_OK) failAllActive(curl_multi_strerror(performed));

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

            if (req->buffer.tooLarge || code == CURLE_FILESIZE_EXCEEDED) {
                req->promise.setError(std::make_exception_ptr(ResponseTooLargeError(req->host, kMaxResponseBytes)));
            } else if (code == CURLE_OK) {
                req->promise.setValue(HttpResponse(status, std::move(req->buffer.body)));
            } else {
                req->promise.setError(std::make_exception_ptr(NetworkError("network error (" + req->host + "): " + message)));
            }
        }

        auto wait = std::chrono::duration_cast<std::chrono::milliseconds>(nextWake - std::chrono::steady_clock::now());
        CURLMcode polled =
            curl_multi_poll(multi, nullptr, 0, static_cast<int>(std::max<long long>(0, wait.count())), nullptr);
        if (polled != CURLM_OK) failAllActive(curl_multi_strerror(polled));
    }
}