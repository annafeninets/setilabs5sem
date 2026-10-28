#include "text_report_renderer.hpp"

#include <iomanip>
#include <sstream>

#include "place_kinds.hpp"

std::string TextReportRenderer::render(const Report& report) const {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1);
    out << "\n=== " << report.location().displayName() << " ===\n\nПогода:\n";

    if (report.weather().ok()) {
        const Weather& w = report.weather().value();
        out << "  " << w.description() << "\n"
            << "  Температура: " << w.temperature() << " °C, ощущается как " << w.feelsLike() << " °C\n"
            << "  Влажность: " << w.humidity() << " %\n"
            << "  Давление: " << w.pressureMmHg() << " мм рт. ст.\n"
            << "  Ветер: " << w.windSpeed() << " м/с\n";
    } else {
        out << "  недоступна (" << report.weather().error() << ")\n";
    }

    out << "\nИнтересные места:\n";
    const auto& places = report.places();
    if (!places.ok()) {
        out << "  недоступны (" << places.error() << ")\n";
    } else if (places.value().empty()) {
        out << "  ничего не найдено\n";
    } else {
        int i = 1;
        for (const Place& p : places.value()) {
            std::string type = describePlaceKinds(p.kinds());
            out << "  " << i++ << ". " << p.name() << " (" << (type.empty() ? "" : type + ", ") << std::setprecision(0)
                << p.distance() << " м)\n" << std::setprecision(1) << "     ";
            switch (p.descriptionStatus()) {
                case Place::DescriptionStatus::Loaded: out << p.description(); break;
                case Place::DescriptionStatus::Missing: out << "(описания нет)"; break;
                case Place::DescriptionStatus::Failed: out << "(описание не удалось загрузить)"; break;
            }
            out << "\n";
        }
    }
    return out.str();
}