#include "place_service.hpp"

#include "util.hpp"

using async::Future;

namespace {

template <class T>
Future<Outcome<T>> attempt(Future<T> f) {
    return f.then([](T v) { return Outcome<T>::success(std::move(v)); })
        .recover([](std::exception_ptr e) { return Outcome<T>::failure(describeException(std::move(e))); });
}

}

Future<std::vector<Location>> PlaceService::findLocations(const std::string& query) {
    return geocoder_.search(query);
}

Future<Place> PlaceService::describePlace(const PlaceRef& ref) {
    return catalog_.describe(ref)
        .then([ref](std::string d) {
            auto status = d.empty() ? Place::DescriptionStatus::Missing : Place::DescriptionStatus::Loaded;
            return Place(ref, std::move(d), status);
        })
        .recover([ref](std::exception_ptr) { return Place(ref, "", Place::DescriptionStatus::Failed); });
}

Future<Report> PlaceService::buildReport(const Location& location) {
    auto weatherF = attempt(weather_.currentWeather(location));

    auto placesF = attempt(catalog_.nearby(location).then([this](std::vector<PlaceRef> refs) {
        std::vector<Future<Place>> details;
        details.reserve(refs.size());
        for (const auto& ref : refs) details.push_back(describePlace(ref));
        return async::whenAll(std::move(details));
    }));

    using Joined = std::pair<Outcome<Weather>, Outcome<std::vector<Place>>>;
    return async::whenBoth(std::move(weatherF), std::move(placesF)).then([location](Joined joined) {
        return Report(location, std::move(joined.first), std::move(joined.second));
    });
}
