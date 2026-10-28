#include "console_app.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>

#include "util.hpp"

namespace {
constexpr std::chrono::seconds kResultTimeout{120};
}

void ConsoleApp::run() {
    std::string line;
    while (true) {
        out_ << "Введите название места (или exit): " << std::flush;
        if (!std::getline(in_, line)) return;
        line = trim(line);
        if (line == "exit") return;
        if (line.empty()) continue;
        try {
            if (!handleQuery(line)) return;
        } catch (const std::exception& e) {
            out_ << "Ошибка: " << e.what() << "\n";
        }
    }
}

bool ConsoleApp::handleQuery(const std::string& query) {
    auto locations = service_.findLocations(query).get(kResultTimeout);
    if (locations.empty()) {
        out_ << "Ничего не найдено.\n";
        return true;
    }
    printLocations(locations);

    bool closed = false;
    auto choice = askChoice(locations.size(), closed);
    if (closed) return false;
    if (!choice) {
        out_ << "Неверный номер.\n";
        return true;
    }

    auto start = std::chrono::steady_clock::now();
    Report report = service_.buildReport(locations[*choice]).get(kResultTimeout);
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    out_ << renderer_.render(report) << "\n(готово за " << std::fixed << std::setprecision(2) << seconds
         << " с)\n\n";
    return true;
}

void ConsoleApp::printLocations(const std::vector<Location>& locations) const {
    out_ << std::fixed << std::setprecision(5);
    for (std::size_t i = 0; i < locations.size(); ++i) {
        out_ << i + 1 << ". " << locations[i].displayName();
        if (!locations[i].type().empty()) out_ << " - " << locations[i].type();
        out_ << " (" << locations[i].latitude() << ", " << locations[i].longitude() << ")\n";
    }
}

std::optional<std::size_t> ConsoleApp::askChoice(std::size_t count, bool& inputClosed) {
    out_ << "Выберите номер локации: " << std::flush;
    std::string line;
    if (!std::getline(in_, line)) {
        inputClosed = true;
        return std::nullopt;
    }
    try {
        std::size_t n = std::stoul(line);
        if (n >= 1 && n <= count) return n - 1;
    } catch (const std::exception&) {
    }
    return std::nullopt;
}