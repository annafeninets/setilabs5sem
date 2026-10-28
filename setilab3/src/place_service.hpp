#pragma once

#include <vector>

#include "future.hpp"
#include "interfaces.hpp"
#include "models.hpp"

class PlaceService {
public:
    PlaceService(IGeocoder& geocoder, IWeatherProvider& weather, IPlaceCatalog& catalog)
        : geocoder_(geocoder), weather_(weather), catalog_(catalog) {}

    async::Future<std::vector<Location>> findLocations(const std::string& query);

    async::Future<Report> buildReport(const Location& location);

private:
    async::Future<Place> describePlace(const PlaceRef& ref);

    IGeocoder& geocoder_;
    IWeatherProvider& weather_;
    IPlaceCatalog& catalog_;
};
