#include "Application.h"
#include "Constants.h"

#include <chrono>
#include <iostream>

namespace
{
    long long elapsedMs(const std::chrono::steady_clock::time_point &start)
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    }
}

Application::Application(const MulticastAddress &group)
    : group_(group), sender_(group), receiver_(group), tracker_(config::TTL_MS)
{
}

bool Application::listenForRound()
{
    bool changed = false;
    auto roundStart = std::chrono::steady_clock::now();

    while (elapsedMs(roundStart) < config::HEARTBEAT_MS)
    {
        auto senderAddress = receiver_.receive(config::MESSAGE);
        if (!senderAddress)
            continue;

        if (tracker_.touch(*senderAddress))
            changed = true;
    }

    return changed;
}

void Application::run()
{
    std::cout << "Слушаю multicast-группу " << group_.text()
               << " (" << (group_.isIPv4() ? "IPv4" : "IPv6")
               << "), порт " << config::PORT << "...\n";

    while (true)
    {
        sender_.send(config::MESSAGE);

        bool changed = listenForRound();
        if (tracker_.removeExpired())
            changed = true;

        if (changed)
            tracker_.print();
    }
}