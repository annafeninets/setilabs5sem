#include "Multicast receiver.h"
#include "Constants.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/time.h>
#include <unistd.h>

namespace setilab1
{
    MulticastReceiver::MulticastReceiver(const MulticastAddress &group)
    {
        socketFd_ = socket(group.family(), SOCK_DGRAM, 0);
        if (socketFd_ < 0)
            throw std::runtime_error("Не удалось создать принимающий сокет");

        int reuse = 1;
        if (setsockopt(socketFd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
            throw std::runtime_error("Не удалось установить SO_REUSEADDR");
    #ifdef SO_REUSEPORT
        setsockopt(socketFd_, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
    #endif

        joinGroup(group);

        timeval timeout{};
        timeout.tv_sec = 0;
        timeout.tv_usec = config::RECV_TIMEOUT_US;
        if (setsockopt(socketFd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
            throw std::runtime_error("Не удалось установить таймаут приёма");
    }

    MulticastReceiver::~MulticastReceiver()
    {
        if (socketFd_ >= 0)
            close(socketFd_);
    }

    void MulticastReceiver::joinGroup(const MulticastAddress &group)
    {
        if (group.isIPv4())
        {
            sockaddr_in bindAddr{};
            bindAddr.sin_family = AF_INET;
            bindAddr.sin_port = htons(config::PORT);
            bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            if (bind(socketFd_, reinterpret_cast<sockaddr *>(&bindAddr), sizeof(bindAddr)) < 0)
                throw std::runtime_error("Не удалось привязать сокет (bind)");

            ip_mreq mreq{};
            mreq.imr_multiaddr.s_addr = inet_addr(group.text().c_str());
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            if (setsockopt(socketFd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
                throw std::runtime_error("Не удалось вступить в multicast-группу (IPv4)");
        }
        else
        {
            sockaddr_in6 bindAddr{};
            bindAddr.sin6_family = AF_INET6;
            bindAddr.sin6_port = htons(config::PORT);
            bindAddr.sin6_addr = in6addr_any;
            if (bind(socketFd_, reinterpret_cast<sockaddr *>(&bindAddr), sizeof(bindAddr)) < 0)
                throw std::runtime_error("Не удалось привязать сокет (bind)");

            ipv6_mreq mreq6{};
            inet_pton(AF_INET6, group.text().c_str(), &mreq6.ipv6mr_multiaddr);
            mreq6.ipv6mr_interface = 0;
            if (setsockopt(socketFd_, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq6, sizeof(mreq6)) < 0)
                throw std::runtime_error("Не удалось вступить в multicast-группу (IPv6)");
        }
    }

    std::optional<std::string> MulticastReceiver::receive(const std::string &expectedMessage) const
    {
        char buffer[256];
        sockaddr_storage sender{};
        socklen_t senderLen = sizeof(sender);

        ssize_t recvLen = recvfrom(socketFd_, buffer, sizeof(buffer) - 1, 0,
                                    reinterpret_cast<sockaddr *>(&sender), &senderLen);

        if (recvLen < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return std::nullopt;
            throw std::runtime_error("Ошибка приёма данных (recvfrom)");
        }

        if (static_cast<size_t>(recvLen) != expectedMessage.size() ||
            memcmp(buffer, expectedMessage.data(), expectedMessage.size()) != 0)
            return std::nullopt;

        return senderToString(sender);
    }

    std::string MulticastReceiver::senderToString(const sockaddr_storage &sender)
    {
        char ipStr[INET6_ADDRSTRLEN] = {0};
        unsigned short port = 0;

        if (sender.ss_family == AF_INET)
        {
            const auto *s4 = reinterpret_cast<const sockaddr_in *>(&sender);
            inet_ntop(AF_INET, &s4->sin_addr, ipStr, sizeof(ipStr));
            port = ntohs(s4->sin_port);
        }
        else if (sender.ss_family == AF_INET6)
        {
            const auto *s6 = reinterpret_cast<const sockaddr_in6 *>(&sender);
            inet_ntop(AF_INET6, &s6->sin6_addr, ipStr, sizeof(ipStr));
            port = ntohs(s6->sin6_port);
        }
        else
        {
            throw std::runtime_error("Неизвестное семейство адреса отправителя");
        }

        return std::string(ipStr) + " : " + std::to_string(port);
    }
}