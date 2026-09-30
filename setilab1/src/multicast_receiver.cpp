#include "multicast_receiver.h"
#include "constants.h"

#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/time.h>

namespace setilab1
{
    MulticastReceiver::MulticastReceiver(const MulticastAddress &group, unsigned int interfaceIndex)
        : socket_(group.family())
    {
        configureReuse();
        joinGroup(group, interfaceIndex);
        configureTimeout();
    }

    void MulticastReceiver::configureReuse()
    {
        int reuse = 1;
        if (setsockopt(socket_.fd(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
            throw std::runtime_error("Не удалось установить SO_REUSEADDR");
#ifdef SO_REUSEPORT
        setsockopt(socket_.fd(), SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
#endif
    }

    void MulticastReceiver::joinGroup(const MulticastAddress &group, unsigned int interfaceIndex)
    {
        if (group.isIPv4())
        {
            sockaddr_in bindAddr{};
            bindAddr.sin_family = AF_INET;
            bindAddr.sin_port = htons(config::PORT);
            bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            if (bind(socket_.fd(), reinterpret_cast<sockaddr *>(&bindAddr), sizeof(bindAddr)) < 0)
                throw std::runtime_error("Не удалось привязать сокет (bind)");

            ip_mreq mreq{};
            if (inet_pton(AF_INET, group.text().c_str(), &mreq.imr_multiaddr) != 1)
                throw std::runtime_error("Не удалось разобрать IPv4-адрес группы");
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            if (setsockopt(socket_.fd(), IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
                throw std::runtime_error("Не удалось вступить в multicast-группу (IPv4)");
        }
        else
        {
            sockaddr_in6 bindAddr{};
            bindAddr.sin6_family = AF_INET6;
            bindAddr.sin6_port = htons(config::PORT);
            bindAddr.sin6_addr = in6addr_any;
            if (bind(socket_.fd(), reinterpret_cast<sockaddr *>(&bindAddr), sizeof(bindAddr)) < 0)
                throw std::runtime_error("Не удалось привязать сокет (bind)");

            ipv6_mreq mreq6{};
            if (inet_pton(AF_INET6, group.text().c_str(), &mreq6.ipv6mr_multiaddr) != 1)
                throw std::runtime_error("Не удалось разобрать IPv6-адрес группы");
            mreq6.ipv6mr_interface = interfaceIndex;
            if (setsockopt(socket_.fd(), IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq6, sizeof(mreq6)) < 0)
                throw std::runtime_error("Не удалось вступить в multicast-группу (IPv6)");
        }
    }

    void MulticastReceiver::configureTimeout()
    {
        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = config::RECV_TIMEOUT_US;
        if (setsockopt(socket_.fd(), SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
            throw std::runtime_error("Не удалось установить таймаут приёма");
    }

    std::optional<Datagram> MulticastReceiver::receive() const
    {
        char buffer[config::MAX_DATAGRAM_SIZE];
        sockaddr_storage sender{};
        socklen_t senderLen = sizeof(sender);

        ssize_t recvLen = recvfrom(socket_.fd(), buffer, sizeof(buffer), 0,
                                   reinterpret_cast<sockaddr *>(&sender), &senderLen);

        if (recvLen < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return std::nullopt;
            throw std::runtime_error("Ошибка приёма данных (recvfrom)");
        }

        return Datagram{std::string(buffer, static_cast<std::size_t>(recvLen)), senderToIp(sender)};
    }

    std::string MulticastReceiver::senderToIp(const sockaddr_storage &sender)
    {
        char ipStr[INET6_ADDRSTRLEN] = {0};

        if (sender.ss_family == AF_INET)
        {
            const auto *s4 = reinterpret_cast<const sockaddr_in *>(&sender);
            inet_ntop(AF_INET, &s4->sin_addr, ipStr, sizeof(ipStr));
        }
        else if (sender.ss_family == AF_INET6)
        {
            const auto *s6 = reinterpret_cast<const sockaddr_in6 *>(&sender);
            inet_ntop(AF_INET6, &s6->sin6_addr, ipStr, sizeof(ipStr));
        }
        else
        {
            throw std::runtime_error("Неизвестное семейство адреса отправителя");
        }

        return ipStr;
    }
}
