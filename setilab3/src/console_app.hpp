#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "interfaces.hpp"
#include "place_service.hpp"

class ConsoleApp {
public:
    ConsoleApp(PlaceService& service, const IReportRenderer& renderer, std::istream& in, std::ostream& out)
        : service_(service), renderer_(renderer), in_(in), out_(out) {}

    void run();

private:
    bool handleQuery(const std::string& query);
    void printLocations(const std::vector<Location>& locations) const;
    struct Choice {
        enum class Kind { Selected, Invalid, Cancelled, Closed };
        Kind kind = Kind::Invalid;
        std::size_t index = 0;
    };

    Choice askChoice(std::size_t count);

    PlaceService& service_;
    const IReportRenderer& renderer_;
    std::istream& in_;
    std::ostream& out_;
};