#include "api_client.hpp"

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
        if (parsed.is_object()) {
            auto it = parsed.find("error");
            if (it != parsed.end() && it->is_string())
                throw ApiError(service, "API error: " + it->get<std::string>());
        }
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