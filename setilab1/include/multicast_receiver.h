#pragma once

#include "multicast_address.h"
#include "udp_socket.h"

#include <optional>
#include <string>
#include <sys/socket.h>

namespace setilab1
{
    struct Datagram
    {
        std::string payload;
        std::string senderIp;
    };

    class MulticastReceiver
    {
    public:
        MulticastReceiver(const MulticastAddress &group, unsigned int interfaceIndex);

        std::optional<Datagram> receive() const;

    private:
        UdpSocket socket_;

        void configureReuse();
        void joinGroup(const MulticastAddress &group, unsigned int interfaceIndex);
        void configureTimeout();
        static std::string senderToIp(const sockaddr_storage &sender);
    };
}
