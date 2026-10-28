#pragma once

#include <string>

class ApiConfig {
public:
    static ApiConfig load(const std::string& envFile);

    const std::string& graphhopperKey() const { return graphhopperKey_; }
    const std::string& openweatherKey() const { return openweatherKey_; }
    const std::string& opentripmapKey() const { return opentripmapKey_; }
    const std::string& graphhopperUrl() const { return graphhopperUrl_; }
    const std::string& nominatimUrl() const { return nominatimUrl_; }
    const std::string& openweatherUrl() const { return openweatherUrl_; }
    const std::string& opentripmapUrl() const { return opentripmapUrl_; }
    bool hasAllKeys() const;

private:
    ApiConfig() = default;

    std::string graphhopperKey_, openweatherKey_, opentripmapKey_;
    std::string graphhopperUrl_ = "https://graphhopper.com/api/1/geocode";
    std::string nominatimUrl_ = "https://nominatim.openstreetmap.org/search";  // запасной геокодер, ключ не нужен
    std::string openweatherUrl_ = "https://api.openweathermap.org/data/2.5/weather";
    std::string opentripmapUrl_ = "https://api.opentripmap.com/0.1/ru/places";
};