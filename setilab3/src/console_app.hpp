#pragma once

#include <iosfwd>
#include <optional>
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
    std::optional<std::size_t> askChoice(std::size_t count, bool& inputClosed);

    PlaceService& service_;
    const IReportRenderer& renderer_;
    std::istream& in_;
    std::ostream& out_;
};
