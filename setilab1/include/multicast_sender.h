#pragma once

#include "multicast_address.h"
#include "udp_socket.h"

#include <netinet/in.h>
#include <string>

namespace setilab1
{
    class MulticastSender
    {
    public:
        MulticastSender(const MulticastAddress &group, unsigned int interfaceIndex);

        void send(const std::string &message) const;

    private:
        UdpSocket socket_;
        sockaddr_storage dest_{};
        socklen_t destLen_ = 0;

        void configureIPv6Interface(unsigned int interfaceIndex);
        void buildDestAddress(const MulticastAddress &group, unsigned int interfaceIndex);
    };
}
