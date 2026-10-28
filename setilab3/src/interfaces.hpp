#pragma once

#include <string>
#include <utility>
#include <vector>

#include "future.hpp"
#include "models.hpp"

class HttpResponse {
public:
    HttpResponse(long status, std::string body) : status_(status), body_(std::move(body)) {}
    long status() const { return status_; }
    const std::string& body() const { return body_; }

private:
    long status_;
    std::string body_;
};

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual async::Future<HttpResponse> get(const std::string& url) = 0;
};

class IGeocoder {
public:
    virtual ~IGeocoder() = default;
    virtual async::Future<std::vector<Location>> search(const std::string& query) = 0;
};

class IWeatherProvider {
public:
    virtual ~IWeatherProvider() = default;
    virtual async::Future<Weather> currentWeather(const Location& location) = 0;
};

class IPlaceCatalog {
public:
    virtual ~IPlaceCatalog() = default;
    virtual async::Future<std::vector<PlaceRef>> nearby(const Location& location) = 0;
    virtual async::Future<std::string> describe(const PlaceRef& place) = 0;
};

class IReportRenderer {
public:
    virtual ~IReportRenderer() = default;
    virtual std::string render(const Report& report) const = 0;
};
