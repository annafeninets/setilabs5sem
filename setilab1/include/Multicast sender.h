#pragma once

#include "Multicast address.h"

#include <netinet/in.h>
#include <string>

namespace setilab1
{
    class MulticastSender
    {
    public:
        explicit MulticastSender(const MulticastAddress &group);
        ~MulticastSender();

        MulticastSender(const MulticastSender &) = delete;
        MulticastSender &operator=(const MulticastSender &) = delete;

        void send(const std::string &message) const;

    private:
        int socketFd_;
        sockaddr_storage dest_{};
        socklen_t destLen_ = 0;

        void buildDestAddress(const MulticastAddress &group);
    };
}