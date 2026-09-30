#include "application.h"
#include "constants.h"
#include "heartbeat.h"
#include "instance_id.h"
#include "network_interface.h"
#include "time_utils.h"

#include <chrono>
#include <iostream>

namespace setilab1
{
    namespace
    {
        unsigned int selectInterfaceIndex(const MulticastAddress &group)
        {
            return group.isIPv6() ? resolveInterfaceIndex(config::NETWORK_INTERFACE) : 0;
        }
    }

    Application::Application(const MulticastAddress &group)
        : group_(group),
          interfaceIndex_(selectInterfaceIndex(group)),
          instanceId_(generateInstanceId()),
          heartbeatPayload_(encodeHeartbeat(instanceId_)),
          sender_(group, interfaceIndex_),
          receiver_(group, interfaceIndex_),
          tracker_(config::TTL_MS)
    {
    }

    bool Application::listenForRound()
    {
        bool changed = false;
        auto roundStart = std::chrono::steady_clock::now();

        while (elapsedMs(roundStart) < config::HEARTBEAT_MS)
        {
            auto datagram = receiver_.receive();
            if (!datagram)
                continue;

            auto senderId = decodeHeartbeat(datagram->payload);
            if (!senderId)
                continue;
            if (*senderId == instanceId_)
                continue;
            if (tracker_.touch(*senderId, datagram->senderIp))
                changed = true;
        }

        return changed;
    }

    void Application::run()
    {
        std::cout << "Слушаю multicast-группу " << group_.text()
                  << " (" << (group_.isIPv4() ? "IPv4" : "IPv6")
                  << "), порт " << config::PORT
                  << ", id этой копии: " << instanceId_ << "...\n"
                  << std::flush;

        while (true)
        {
            sender_.send(heartbeatPayload_);

            bool changed = listenForRound();
            if (tracker_.removeExpired())
                changed = true;

            if (changed)
                tracker_.print();
        }
    }
}


