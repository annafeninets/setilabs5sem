#pragma once

#include <optional>
#include <string>

namespace setilab1
{
    std::string encodeHeartbeat(const std::string &instanceId);

    std::optional<std::string> decodeHeartbeat(const std::string &payload);
}
