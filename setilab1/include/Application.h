#pragma once

#include "Multicast address.h"
#include "Multicast receiver.h"
#include "Multicast sender.h"
#include "Peer tracker.h"

namespace setilab1
{
    class Application
    {
    public:
        explicit Application(const MulticastAddress &group);

        void run();

    private:
        MulticastAddress group_;
        MulticastSender sender_;
        MulticastReceiver receiver_;
        PeerTracker tracker_;

        bool listenForRound();
    };
}