#include "peer_tracker.h"
#include "time_utils.h"

#include <iostream>

namespace setilab1
{
    PeerTracker::PeerTracker(int ttlMs) : ttlMs_(ttlMs) {}

    bool PeerTracker::touch(const std::string &instanceId, const std::string &ip)
    {
        auto it = peers_.find(instanceId);
        bool changed = (it == peers_.end()) || (it->second.ip != ip);

        peers_[instanceId] = Peer{ip, std::chrono::steady_clock::now()};
        return changed;
    }

    bool PeerTracker::removeExpired()
    {
        bool changed = false;
        for (auto it = peers_.begin(); it != peers_.end();)
        {
            if (elapsedMs(it->second.lastSeen) > ttlMs_)
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
        std::cout << "Живых копий (кроме этой): " << peers_.size() << "\n";
        for (const auto &entry : peers_)
            std::cout << "  " << entry.second.ip << "  (id " << entry.first << ")\n";
        std::cout << std::flush;
    }
}
