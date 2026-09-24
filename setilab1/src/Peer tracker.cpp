#include "Peer tracker.h"

#include <iostream>

PeerTracker::PeerTracker(int ttlMs) : ttlMs_(ttlMs) {}

bool PeerTracker::touch(const std::string &peerAddress)
{
    bool isNew = peers_.find(peerAddress) == peers_.end();
    peers_[peerAddress] = std::chrono::steady_clock::now();
    return isNew;
}

bool PeerTracker::removeExpired()
{
    bool changed = false;
    for (auto it = peers_.begin(); it != peers_.end();)
    {
        if (elapsedMs(it->second) > ttlMs_)
        {
            it = peers_.erase(it);
            changed = true;
        }
        else
        {
            ++it;
        }
    }
    return changed;
}

void PeerTracker::print() const
{
    std::cout << "Живых копий: " << peers_.size() << "\n";
    for (const auto &entry : peers_)
        std::cout << "  " << entry.first << "\n";
}

long long PeerTracker::elapsedMs(const std::chrono::steady_clock::time_point &start)
{
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}