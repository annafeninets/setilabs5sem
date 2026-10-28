#pragma once

#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class Location {
public:
    Location(std::string displayName, double latitude, double longitude, std::string type = "")
        : displayName_(std::move(displayName)), type_(std::move(type)), latitude_(latitude), longitude_(longitude) {}

    const std::string& displayName() const { return displayName_; }
    const std::string& type() const { return type_; }
    double latitude() const { return latitude_; }
    double longitude() const { return longitude_; }

private:
    std::string displayName_, type_;
    double latitude_, longitude_;
};

class Weather {
public:
    static constexpr double kHectopascalToMmHg = 0.750062;

    Weather(std::string city, std::string description, double temp, double feelsLike, int humidity,
            int pressure, double windSpeed)
        : city_(std::move(city)), description_(std::move(description)), temp_(temp), feelsLike_(feelsLike),
          humidity_(humidity), pressure_(pressure), windSpeed_(windSpeed) {}

    const std::string& city() const { return city_; }
    const std::string& description() const { return description_; }
    double temperature() const { return temp_; }
    double feelsLike() const { return feelsLike_; }
    int humidity() const { return humidity_; }
    int pressure() const { return pressure_; }
    int pressureMmHg() const { return static_cast<int>(std::lround(pressure_ * kHectopascalToMmHg)); }
    double windSpeed() const { return windSpeed_; }

private:
    std::string city_, description_;
    double temp_, feelsLike_;
    int humidity_, pressure_;
    double windSpeed_;
};

class PlaceRef {
public:
    PlaceRef(std::string xid, std::string name, std::string kinds, double distance)
        : xid_(std::move(xid)), name_(std::move(name)), kinds_(std::move(kinds)), distance_(distance) {}

    const std::string& xid() const { return xid_; }
    const std::string& name() const { return name_; }
    const std::string& kinds() const { return kinds_; }
    double distance() const { return distance_; }

private:
    std::string xid_, name_, kinds_;
    double distance_;
};

class Place {
public:
    enum class DescriptionStatus { Loaded, Missing, Failed };

    Place(PlaceRef ref, std::string description, DescriptionStatus status)
        : ref_(std::move(ref)), description_(std::move(description)), status_(status) {}

    const std::string& name() const { return ref_.name(); }
    const std::string& kinds() const { return ref_.kinds(); }
    double distance() const { return ref_.distance(); }
    const std::string& description() const { return description_; }
    DescriptionStatus descriptionStatus() const { return status_; }

private:
    PlaceRef ref_;
    std::string description_;
    DescriptionStatus status_;
};

template <class T>
class Outcome {
public:
    static Outcome success(T value) { return Outcome(std::optional<T>(std::move(value)), ""); }
    static Outcome failure(std::string error) { return Outcome(std::nullopt, std::move(error)); }

    bool ok() const { return value_.has_value(); }
    const T& value() const { return *value_; }
    const std::string& error() const { return error_; }

private:
    Outcome(std::optional<T> value, std::string error) : value_(std::move(value)), error_(std::move(error)) {}

    std::optional<T> value_;
    std::string error_;
};

class Report {
public:
    Report(Location location, Outcome<Weather> weather, Outcome<std::vector<Place>> places)
        : location_(std::move(location)), weather_(std::move(weather)), places_(std::move(places)) {}

    const Location& location() const { return location_; }
    const Outcome<Weather>& weather() const { return weather_; }
    const Outcome<std::vector<Place>>& places() const { return places_; }

private:
    Location location_;
    Outcome<Weather> weather_;
    Outcome<std::vector<Place>> places_;
};