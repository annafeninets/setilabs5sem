#pragma once

#include <chrono>
#include <map>
#include <string>

namespace setilab1
{
    class PeerTracker
    {
    public:
        explicit PeerTracker(int ttlMs);

        bool touch(const std::string &instanceId, const std::string &ip);

        bool removeExpired();

        void print() const;

    private:
        struct Peer
        {
            std::string ip;
            std::chrono::steady_clock::time_point lastSeen;
        };

        int ttlMs_;
        std::map<std::string, Peer> peers_;
    };
}
