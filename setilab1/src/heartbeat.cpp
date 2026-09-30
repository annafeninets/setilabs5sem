#include "heartbeat.h"
#include "constants.h"

namespace setilab1
{
    namespace
    {
        constexpr std::size_t MAX_ID_LENGTH = 64;
    }

    std::string encodeHeartbeat(const std::string &instanceId)
    {
        return std::string(config::MESSAGE) + config::HEARTBEAT_SEPARATOR + instanceId;
    }

    std::optional<std::string> decodeHeartbeat(const std::string &payload)
    {
        const std::string prefix = std::string(config::MESSAGE) + config::HEARTBEAT_SEPARATOR;

        if (payload.size() <= prefix.size() || payload.compare(0, prefix.size(), prefix) != 0)
            return std::nullopt;

        std::string id = payload.substr(prefix.size());
        if (id.size() > MAX_ID_LENGTH)
            return std::nullopt;

        return id;
    }
}
