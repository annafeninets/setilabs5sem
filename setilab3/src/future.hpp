#pragma once

#include <chrono>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace async {

template <class T> class Future;
template <class T> class Promise;

class TimeoutError : public std::runtime_error {
public:
    TimeoutError() : std::runtime_error("timed out waiting for result") {}
};

namespace detail {

template <class T> struct IsFuture : std::false_type {};
template <class T> struct IsFuture<Future<T>> : std::true_type {};
template <class T> struct Unwrap { using type = T; };
template <class T> struct Unwrap<Future<T>> { using type = T; };

template <class T>
struct State {
    std::mutex mutex;
    std::condition_variable cv;
    bool ready = false;
    std::optional<T> value;
    std::exception_ptr error;
    std::vector<std::function<void()>> callbacks;
};

}

template <class T>
class Promise {
public:
    Promise() : state_(std::make_shared<detail::State<T>>()) {}

    Future<T> future() const;

    void setValue(T value) const {
        complete([&] { state_->value.emplace(std::move(value)); });
    }
    void setError(std::exception_ptr error) const {
        complete([&] { state_->error = std::move(error); });
    }

private:
    template <class Store>
    void complete(Store&& store) const {
        std::vector<std::function<void()>> callbacks;
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (state_->ready) return;
            store();
            state_->ready = true;
            callbacks.swap(state_->callbacks);
        }
        state_->cv.notify_all();
        for (auto& cb : callbacks) cb();
    }

    std::shared_ptr<detail::State<T>> state_;
};

template <class T>
class Future {
public:
    using value_type = T;

    Future() = default;

    T get() {
        std::unique_lock<std::mutex> lock(state_->mutex);
        state_->cv.wait(lock, [this] { return state_->ready; });
        if (state_->error) std::rethrow_exception(state_->error);
        return std::move(*state_->value);
    }

    template <class Rep, class Period>
    T get(std::chrono::duration<Rep, Period> timeout) {
        std::unique_lock<std::mutex> lock(state_->mutex);
        if (!state_->cv.wait_for(lock, timeout, [this] { return state_->ready; })) throw TimeoutError();
        if (state_->error) std::rethrow_exception(state_->error);
        return std::move(*state_->value);
    }

    template <class OnValue, class OnError>
    void subscribe(OnValue ok, OnError err) {
        onReady([st = state_, ok = std::move(ok), err = std::move(err)]() mutable {
            if (st->error) err(st->error);
            else ok(std::move(*st->value));
        });
    }

    template <class F>
    auto then(F f) -> Future<typename detail::Unwrap<std::invoke_result_t<F, T>>::type> {
        using R = std::invoke_result_t<F, T>;
        using U = typename detail::Unwrap<R>::type;
        Promise<U> promise;
        subscribe(
            [promise, f = std::move(f)](T&& value) mutable {
                try {
                    if constexpr (detail::IsFuture<R>::value) {
                        R inner = f(std::move(value));
                        inner.subscribe(
                            [promise](U&& u) { promise.setValue(std::move(u)); },
                            [promise](std::exception_ptr e) { promise.setError(std::move(e)); });
                    } else {
                        promise.setValue(f(std::move(value)));
                    }
                } catch (...) {
                    promise.setError(std::current_exception());
                }
            },
            [promise](std::exception_ptr e) { promise.setError(std::move(e)); });
        return promise.future();
    }

    template <class F>
    Future<T> recover(F f) {
        Promise<T> promise;
        subscribe(
            [promise](T&& value) { promise.setValue(std::move(value)); },
            [promise, f = std::move(f)](std::exception_ptr e) mutable {
                try {
                    promise.setValue(f(std::move(e)));
                } catch (...) {
                    promise.setError(std::current_exception());
                }
            });
        return promise.future();
    }

    template <class F>
    Future<T> recoverWith(F f) {
        Promise<T> promise;
        subscribe(
            [promise](T&& value) { promise.setValue(std::move(value)); },
            [promise, f = std::move(f)](std::exception_ptr e) mutable {
                try {
                    f(std::move(e)).subscribe([promise](T&& v) { promise.setValue(std::move(v)); },
                                              [promise](std::exception_ptr e2) { promise.setError(std::move(e2)); });
                } catch (...) {
                    promise.setError(std::current_exception());
                }
            });
        return promise.future();
    }

private:
    friend class Promise<T>;
    explicit Future(std::shared_ptr<detail::State<T>> state) : state_(std::move(state)) {}

    void onReady(std::function<void()> cb) {
        {
            std::lock_guard<std::mutex> lock(state_->mutex);
            if (!state_->ready) {
                state_->callbacks.push_back(std::move(cb));
                return;
            }
        }
        cb();
    }

    std::shared_ptr<detail::State<T>> state_;
};

template <class T>
Future<T> Promise<T>::future() const {
    return Future<T>(state_);
}

template <class T>
Future<T> makeReadyFuture(T value) {
    Promise<T> p;
    p.setValue(std::move(value));
    return p.future();
}

template <class T>
Future<T> makeFailedFuture(std::exception_ptr error) {
    Promise<T> p;
    p.setError(std::move(error));
    return p.future();
}

template <class T>
Future<std::vector<T>> whenAll(std::vector<Future<T>> futures) {
    struct Shared {
        std::mutex mutex;
        std::vector<std::optional<T>> results;
        std::size_t remaining = 0;
        std::exception_ptr error;
    };
    Promise<std::vector<T>> promise;
    if (futures.empty()) {
        promise.setValue({});
        return promise.future();
    }
    auto shared = std::make_shared<Shared>();
    shared->results.resize(futures.size());
    shared->remaining = futures.size();

    auto finishOne = [shared, promise](std::size_t index, std::optional<T> value, std::exception_ptr error) {
        bool last = false;
        {
            std::lock_guard<std::mutex> lock(shared->mutex);
            if (error) {
                if (!shared->error) shared->error = std::move(error);
            } else {
                shared->results[index] = std::move(value);
            }
            last = (--shared->remaining == 0);
        }
        if (!last) return;
        if (shared->error) {
            promise.setError(shared->error);
            return;
        }
        std::vector<T> out;
        out.reserve(shared->results.size());
        for (auto& r : shared->results) out.push_back(std::move(*r));
        promise.setValue(std::move(out));
    };

    for (std::size_t i = 0; i < futures.size(); ++i) {
        futures[i].subscribe(
            [finishOne, i](T&& v) { finishOne(i, std::optional<T>(std::move(v)), nullptr); },
            [finishOne, i](std::exception_ptr e) { finishOne(i, std::nullopt, std::move(e)); });
    }
    return promise.future();
}

template <class A, class B>
Future<std::pair<A, B>> whenBoth(Future<A> a, Future<B> b) {
    struct Shared {
        std::mutex mutex;
        std::optional<A> first;
        std::optional<B> second;
        std::exception_ptr error;
        int remaining = 2;
    };
    Promise<std::pair<A, B>> promise;
    auto shared = std::make_shared<Shared>();

    auto finish = [shared, promise]() {
        if (shared->error) promise.setError(shared->error);
        else promise.setValue(std::make_pair(std::move(*shared->first), std::move(*shared->second)));
    };
    auto onError = [shared, finish](std::exception_ptr e) {
        bool last;
        {
            std::lock_guard<std::mutex> lock(shared->mutex);
            if (!shared->error) shared->error = std::move(e);
            last = (--shared->remaining == 0);
        }
        if (last) finish();
    };

    a.subscribe(
        [shared, finish](A&& v) {
            bool last;
            {
                std::lock_guard<std::mutex> lock(shared->mutex);
                shared->first.emplace(std::move(v));
                last = (--shared->remaining == 0);
            }
            if (last) finish();
        },
        onError);
    b.subscribe(
        [shared, finish](B&& v) {
            bool last;
            {
                std::lock_guard<std::mutex> lock(shared->mutex);
                shared->second.emplace(std::move(v));
                last = (--shared->remaining == 0);
            }
            if (last) finish();
        },
        onError);
    return promise.future();
}

}