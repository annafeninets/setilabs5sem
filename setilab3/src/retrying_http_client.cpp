#include "retrying_http_client.hpp"

#include "errors.hpp"
#include "future.hpp"
#include "interfaces.hpp"

namespace {

    bool isTransient(const std::exception_ptr& e) {
        try {
            std::rethrow_exception(e);
        } catch (const NetworkError&) {
            return true;
        } catch (...) {
            return false;
        }
    }

}

async::Future<HttpResponse> RetryingHttpClient::attempt(const std::string& url, int retriesLeft) {
    return inner_.get(url).recoverWith([this, url, retriesLeft](std::exception_ptr e) {
        if (retriesLeft > 0 && isTransient(e)) return attempt(url, retriesLeft - 1);
        return async::makeFailedFuture<HttpResponse>(std::move(e));
    });
}