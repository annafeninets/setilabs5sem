#include "console_app.hpp"

#include <charconv>
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

    std::size_t selected = 0;
    while (true) {
        Choice choice = askChoice(locations.size());
        if (choice.kind == Choice::Kind::Closed) return false;
        if (choice.kind == Choice::Kind::Cancelled) return true;
        if (choice.kind == Choice::Kind::Selected) {
            selected = choice.index;
            break;
        }
        out_ << "Неверный номер. Введите число от 1 до " << locations.size() << " или Enter для нового поиска.\n";
    }

    auto start = std::chrono::steady_clock::now();
    Report report = service_.buildReport(locations[selected]).get(kResultTimeout);
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

ConsoleApp::Choice ConsoleApp::askChoice(std::size_t count) {
    out_ << "Выберите номер локации (Enter - новый поиск): " << std::flush;
    std::string line;
    if (!std::getline(in_, line)) return {Choice::Kind::Closed, 0};
    line = trim(line);
    if (line.empty()) return {Choice::Kind::Cancelled, 0};

    std::size_t number = 0;
    auto [end, error] = std::from_chars(line.data(), line.data() + line.size(), number);
    if (error != std::errc() || end != line.data() + line.size() || number < 1 || number > count)
        return {Choice::Kind::Invalid, 0};
    return {Choice::Kind::Selected, number - 1};
}