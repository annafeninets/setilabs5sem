#pragma once

#include <chrono>

namespace setilab1
{
    inline long long elapsedMs(const std::chrono::steady_clock::time_point &start)
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    }
}
