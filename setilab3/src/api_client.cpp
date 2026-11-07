#include "api_client.hpp"

#include <algorithm>
#include <cctype>

#include "errors.hpp"
#include "util.hpp"

using nlohmann::json;

async::Future<json> ApiClient::getJson(const std::string& url) const {
    std::string service = serviceName();
    return http_.get(url).then([service](HttpResponse response) -> json {
        if (response.status() != 200)
            throw HttpStatusError(service, response.status(), truncateUtf8(trim(response.body()), 120));
        json parsed;
        try {
            parsed = json::parse(response.body());
        } catch (const json::exception& e) {
            throw ApiError(service, std::string("bad JSON: ") + e.what());
        }
        if (auto apiError = findApiError(parsed)) throw ApiError(service, "API error: " + *apiError);
        return parsed;
    });
}

std::string ApiClient::stringField(const json& j, const char* key) {
    auto it = j.find(key);
    return (it != j.end() && it->is_string()) ? it->get<std::string>() : "";
}

double ApiClient::numberField(const json& j, const char* key, double fallback) {
    auto it = j.find(key);
    return (it != j.end() && it->is_number()) ? it->get<double>() : fallback;
}

namespace {

constexpr int kOkStatusCode = 200;

std::string describeErrorValue(const nlohmann::json& value) {
    if (value.is_string()) return value.get<std::string>();
    if (value.is_object()) {
        auto it = value.find("message");
        if (it != value.end() && it->is_string()) return it->get<std::string>();
    }
    return value.dump();
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool readStatusCode(const nlohmann::json& value, int& code) {
    try {
        if (value.is_number()) {
            code = static_cast<int>(value.get<double>());
            return true;
        }
        if (value.is_string()) {
            code = std::stoi(value.get<std::string>());
            return true;
        }
    } catch (const std::exception&) {
    }
    return false;
}

}

std::optional<std::string> ApiClient::findApiError(const json& j) {
    if (!j.is_object()) return std::nullopt;

    auto error = j.find("error");
    if (error != j.end() && !error->is_null() && !(error->is_boolean() && !error->get<bool>()) &&
        !(error->is_string() && error->get<std::string>().empty()))
        return describeErrorValue(*error);

    auto errors = j.find("errors");
    if (errors != j.end() && errors->is_array() && !errors->empty()) return describeErrorValue(errors->front());

    auto cod = j.find("cod");
    int code = 0;
    if (cod != j.end() && readStatusCode(*cod, code) && code != kOkStatusCode) {
        std::string message = stringField(j, "message");
        return "code " + std::to_string(code) + (message.empty() ? "" : ": " + message);
    }

    auto status = j.find("status");
    if (status != j.end() && status->is_string()) {
        std::string value = toLower(status->get<std::string>());
        if (value == "error" || value == "fail" || value == "failed") {
            std::string message = stringField(j, "message");
            return message.empty() ? value : message;
        }
    }

    if (j.contains("hints") && j.contains("message") && j["message"].is_string())
        return j["message"].get<std::string>();

    return std::nullopt;
}

const json& ApiClient::requireObject(const std::string& service, const json& parent, const char* key) {
    auto it = parent.find(key);
    if (it == parent.end() || !it->is_object())
        throw ApiError(service, std::string("missing or invalid object '") + key + "' in response");
    return *it;
}

double ApiClient::requireNumber(const std::string& service, const json& object, const char* key, const char* path) {
    auto it = object.find(key);
    if (it == object.end() || !it->is_number())
        throw ApiError(service, std::string("missing or invalid field '") + path + "' in response");
    return it->get<double>();
}