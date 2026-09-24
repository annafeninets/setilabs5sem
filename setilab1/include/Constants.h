#pragma once

namespace config
{
    constexpr int PORT = 8080;
    constexpr const char *NETWORK_INTERFACE = "enp0s3";

    constexpr int HEARTBEAT_MS = 1000;
    constexpr int TTL_MS = 2000;
    constexpr int RECV_TIMEOUT_US = 200000;
    constexpr const char *MESSAGE = "tosi bosi";
}