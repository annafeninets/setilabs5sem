#pragma once

#include <nlohmann/json.hpp>

#include <optional>
#include <string>

#include "future.hpp"
#include "interfaces.hpp"

class ApiClient {
public:
    virtual ~ApiClient() = default;

protected:
    ApiClient(IHttpClient& http, std::string baseUrl, std::string apiKey)
        : http_(http), baseUrl_(std::move(baseUrl)), apiKey_(std::move(apiKey)) {}

    virtual std::string serviceName() const = 0;

    const std::string& baseUrl() const { return baseUrl_; }
    const std::string& apiKey() const { return apiKey_; }

    async::Future<nlohmann::json> getJson(const std::string& url) const;

    static std::string stringField(const nlohmann::json& j, const char* key);
    static double numberField(const nlohmann::json& j, const char* key, double fallback = 0);

    static std::optional<std::string> findApiError(const nlohmann::json& j);
    static const nlohmann::json& requireObject(const std::string& service, const nlohmann::json& parent, const char* key);
    static double requireNumber(const std::string& service, const nlohmann::json& object, const char* key,
                                const char* path);

private:
    IHttpClient& http_;
    std::string baseUrl_;
    std::string apiKey_;
};