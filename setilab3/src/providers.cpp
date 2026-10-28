#include "providers.hpp"

#include <algorithm>

#include "place_kinds.hpp"
#include "url_builder.hpp"
#include "util.hpp"

using nlohmann::json;

namespace {

constexpr std::size_t kMaxPlaces = 5;
constexpr std::size_t kMaxDescriptionChars = 300;

std::string joinNonEmpty(const std::vector<std::string>& parts) {
    std::string out;
    for (const auto& p : parts) {
        if (p.empty()) continue;
        if (!out.empty()) out += ", ";
        out += p;
    }
    return out;
}

bool readCoordinate(const json& j, const char* key, double& out) {
    auto it = j.find(key);
    if (it == j.end()) return false;
    try {
        if (it->is_number()) out = it->get<double>();
        else if (it->is_string()) out = std::stod(it->get<std::string>());
        else return false;
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

int rateOf(const json& j) {
    auto it = j.find("rate");
    if (it == j.end()) return 0;
    if (it->is_number()) return static_cast<int>(it->get<double>());
    if (it->is_string() && !it->get<std::string>().empty()) {
        char c = it->get<std::string>()[0];
        if (c >= '0' && c <= '9') return c - '0';
    }
    return 0;
}

}

async::Future<std::vector<Location>> GraphHopperGeocoder::search(const std::string& query) {
    auto url = UrlBuilder(baseUrl())
                   .param("q", query)
                   .param("locale", "ru")
                   .param("limit", "7")
                   .param("key", apiKey())
                   .build();
    return getJson(url).then([](json j) {
        std::vector<Location> result;
        for (const auto& hit : j.value("hits", json::array())) {
            if (!hit.contains("point") || !hit["point"].is_object()) continue;
            std::string name = joinNonEmpty({stringField(hit, "name"),
                                             trim(stringField(hit, "street") + " " + stringField(hit, "housenumber")),
                                             stringField(hit, "city"), stringField(hit, "state"),
                                             stringField(hit, "country"), stringField(hit, "postcode")});
            result.emplace_back(std::move(name), numberField(hit["point"], "lat"), numberField(hit["point"], "lng"),
                                describeOsmFeature(stringField(hit, "osm_key"), stringField(hit, "osm_value")));
        }
        return result;
    });
}

async::Future<std::vector<Location>> NominatimGeocoder::search(const std::string& query) {
    auto url = UrlBuilder(baseUrl())
                   .param("q", query)
                   .param("format", "jsonv2")
                   .param("accept-language", "ru")
                   .param("limit", "7")
                   .build();
    return getJson(url).then([](json j) {
        std::vector<Location> result;
        if (!j.is_array()) return result;
        for (const auto& item : j) {
            double lat = 0, lon = 0;
            if (!readCoordinate(item, "lat", lat) || !readCoordinate(item, "lon", lon)) continue;
            std::string name = stringField(item, "display_name");
            if (name.empty()) name = stringField(item, "name");
            std::string category = stringField(item, "category");
            if (category.empty()) category = stringField(item, "class");
            result.emplace_back(std::move(name), lat, lon, describeOsmFeature(category, stringField(item, "type")));
        }
        return result;
    });
}

async::Future<std::vector<Location>> FallbackGeocoder::search(const std::string& query) {
    return primary_.search(query).recoverWith([this, query](std::exception_ptr) { return fallback_.search(query); });
}

async::Future<Weather> OpenWeatherProvider::currentWeather(const Location& location) {
    auto url = UrlBuilder(baseUrl())
                   .param("lat", location.latitude())
                   .param("lon", location.longitude())
                   .param("units", "metric")
                   .param("lang", "ru")
                   .param("appid", apiKey())
                   .build();
    return getJson(url).then([](json j) {
        std::string description;
        if (j.contains("weather") && j["weather"].is_array() && !j["weather"].empty())
            description = stringField(j["weather"][0], "description");
        double temp = 0, feels = 0, humidity = 0, pressure = 0, wind = 0;
        if (j.contains("main") && j["main"].is_object()) {
            temp = numberField(j["main"], "temp");
            feels = numberField(j["main"], "feels_like");
            humidity = numberField(j["main"], "humidity");
            pressure = numberField(j["main"], "pressure");
        }
        if (j.contains("wind") && j["wind"].is_object()) wind = numberField(j["wind"], "speed");
        return Weather(stringField(j, "name"), description, temp, feels, static_cast<int>(humidity),
                       static_cast<int>(pressure), wind);
    });
}

async::Future<std::vector<PlaceRef>> OpenTripMapCatalog::nearby(const Location& location) {
    auto url = UrlBuilder(baseUrl() + "/radius")
                   .param("lon", location.longitude())
                   .param("lat", location.latitude())
                   .param("radius", "5000")
                   .param("limit", "50")
                   .param("format", "json")
                   .param("apikey", apiKey())
                   .build();
    return getJson(url).then([](json j) {
        std::vector<json> items;
        if (j.is_array()) {
            items.assign(j.begin(), j.end());
        } else if (j.is_object() && j.contains("features")) {
            for (const auto& f : j["features"])
                if (f.contains("properties")) items.push_back(f["properties"]);
        }
        std::vector<std::pair<int, PlaceRef>> named;
        for (const auto& it : items) {
            std::string xid = stringField(it, "xid"), name = stringField(it, "name");
            if (xid.empty() || name.empty()) continue;
            named.emplace_back(rateOf(it), PlaceRef(std::move(xid), std::move(name), stringField(it, "kinds"),
                                                     numberField(it, "dist")));
        }
        std::stable_sort(named.begin(), named.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        std::vector<PlaceRef> result;
        for (auto& entry : named) {
            result.push_back(std::move(entry.second));
            if (result.size() == kMaxPlaces) break;
        }
        return result;
    });
}

async::Future<std::string> OpenTripMapCatalog::describe(const PlaceRef& place) {
    auto url = UrlBuilder(baseUrl() + "/xid").path(place.xid()).param("apikey", apiKey()).build();
    return getJson(url).then([](json j) {
        std::string text;
        if (j.contains("wikipedia_extracts") && j["wikipedia_extracts"].is_object())
            text = stringField(j["wikipedia_extracts"], "text");
        if (text.empty() && j.contains("info") && j["info"].is_object()) text = stringField(j["info"], "descr");
        return truncateUtf8(stripHtml(text), kMaxDescriptionChars);
    });
}