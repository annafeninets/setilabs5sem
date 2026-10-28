#pragma once

#include <stdexcept>
#include <string>

class AppError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class NetworkError : public AppError {
public:
    using AppError::AppError;
};

class ApiError : public AppError {
public:
    ApiError(std::string service, const std::string& message)
        : AppError(service + ": " + message), service_(std::move(service)) {}
    const std::string& service() const { return service_; }

private:
    std::string service_;
};

class HttpStatusError : public ApiError {
public:
    HttpStatusError(std::string service, long status, const std::string& body)
        : ApiError(std::move(service), "HTTP " + std::to_string(status) + " " + body), status_(status) {}
    long status() const { return status_; }

private:
    long status_;
};
