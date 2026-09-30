#include "instance_id.h"

#include <cstdint>
#include <cstdio>
#include <random>

namespace setilab1
{
    std::string generateInstanceId()
    {
        std::random_device rd;
        std::uint64_t value = (static_cast<std::uint64_t>(rd()) << 32) | rd();

        char buffer[17];
        std::snprintf(buffer, sizeof(buffer), "%016llx", static_cast<unsigned long long>(value));
        return buffer;
    }
}
