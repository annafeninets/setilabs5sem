#include "retrying_http_client.hpp"

#include "errors.hpp"
#include "future.hpp"
#include "interfaces.hpp"

namespace {

constexpr long kStatusTooManyRequests = 429;
constexpr long kStatusBadGateway = 502;
constexpr long kStatusServiceUnavailable = 503;
constexpr long kStatusGatewayTimeout = 504;
constexpr std::chrono::milliseconds kHttpRetryBaseDelay{1000};

bool isTransient(const std::exception_ptr& e) {
    try {
        std::rethrow_exception(e);
    } catch (const NetworkError&) {
        return true;
    } catch (...) {
        return false;
    }
}

bool isRetryableStatus(long status) {
    return status == kStatusTooManyRequests || status == kStatusBadGateway ||
           status == kStatusServiceUnavailable || status == kStatusGatewayTimeout;
}

std::chrono::milliseconds backoffDelay(int attemptIndex) {
    return kHttpRetryBaseDelay * (1 << attemptIndex);
}

void forward(async::Future<HttpResponse> source, async::Promise<HttpResponse> target) {
    source.subscribe([target](const HttpResponse& r) { target.setValue(r); },
                     [target](std::exception_ptr e) { target.setError(std::move(e)); });
}

}

async::Future<HttpResponse> RetryingHttpClient::attempt(const std::string& url, int retriesLeft, int attemptIndex,
                                                         std::chrono::milliseconds delay) {
    async::Promise<HttpResponse> promise;
    inner_.getAfter(url, delay).subscribe(
        [this, promise, url, retriesLeft, attemptIndex](const HttpResponse& response) {
            if (retriesLeft > 0 && isRetryableStatus(response.status())) {
                forward(attempt(url, retriesLeft - 1, attemptIndex + 1, backoffDelay(attemptIndex)), promise);
            } else {
                promise.setValue(response);
            }
        },
        [this, promise, url, retriesLeft, attemptIndex](std::exception_ptr error) {
            if (retriesLeft > 0 && isTransient(error)) {
                forward(attempt(url, retriesLeft - 1, attemptIndex + 1, std::chrono::milliseconds(0)), promise);
            } else {
                promise.setError(std::move(error));
            }
        });
    return promise.future();
}