#pragma once

#include "api_client.hpp"
#include "interfaces.hpp"

class GraphHopperGeocoder final : public ApiClient, public IGeocoder {
public:
    GraphHopperGeocoder(IHttpClient& http, std::string baseUrl, std::string apiKey)
        : ApiClient(http, std::move(baseUrl), std::move(apiKey)) {}

    async::Future<std::vector<Location>> search(const std::string& query) override;

protected:
    std::string serviceName() const override { return "GraphHopper"; }
};

class NominatimGeocoder final : public ApiClient, public IGeocoder {
public:
    NominatimGeocoder(IHttpClient& http, std::string baseUrl)
        : ApiClient(http, std::move(baseUrl), "") {}

    async::Future<std::vector<Location>> search(const std::string& query) override;

protected:
    std::string serviceName() const override { return "Nominatim"; }
};

class FallbackGeocoder final : public IGeocoder {
public:
    FallbackGeocoder(IGeocoder& primary, IGeocoder& fallback) : primary_(primary), fallback_(fallback) {}

    async::Future<std::vector<Location>> search(const std::string& query) override;

private:
    IGeocoder& primary_;
    IGeocoder& fallback_;
};

class OpenWeatherProvider final : public ApiClient, public IWeatherProvider {
public:
    OpenWeatherProvider(IHttpClient& http, std::string baseUrl, std::string apiKey)
        : ApiClient(http, std::move(baseUrl), std::move(apiKey)) {}

    async::Future<Weather> currentWeather(const Location& location) override;

protected:
    std::string serviceName() const override { return "OpenWeather"; }
};

class OpenTripMapCatalog final : public ApiClient, public IPlaceCatalog {
public:
    OpenTripMapCatalog(IHttpClient& http, std::string baseUrl, std::string apiKey)
        : ApiClient(http, std::move(baseUrl), std::move(apiKey)) {}

    async::Future<std::vector<PlaceRef>> nearby(const Location& location) override;
    async::Future<std::string> describe(const PlaceRef& place) override;

protected:
    std::string serviceName() const override { return "OpenTripMap"; }
};