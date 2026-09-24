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

        bool touch(const std::string &peerAddress);

        bool removeExpired();

        void print() const;

    private:
        int ttlMs_;
        std::map<std::string, std::chrono::steady_clock::time_point> peers_;

        static long long elapsedMs(const std::chrono::steady_clock::time_point &start);
    };
}