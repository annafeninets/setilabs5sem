#pragma once

#include "multicast_address.h"
#include "multicast_receiver.h"
#include "multicast_sender.h"
#include "peer_tracker.h"

#include <string>

namespace setilab1
{
    class Application
    {
    public:
        explicit Application(const MulticastAddress &group);

        void run();

    private:
        MulticastAddress group_;
        unsigned int interfaceIndex_;
        std::string instanceId_;
        std::string heartbeatPayload_;
        MulticastSender sender_;
        MulticastReceiver receiver_;
        PeerTracker tracker_;

        bool listenForRound();
    };
}
