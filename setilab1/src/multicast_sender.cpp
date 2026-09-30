#include "multicast_sender.h"
#include "constants.h"

#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>

namespace setilab1
{
    MulticastSender::MulticastSender(const MulticastAddress &group, unsigned int interfaceIndex)
        : socket_(group.family())
    {
        if (group.isIPv6())
            configureIPv6Interface(interfaceIndex);

        buildDestAddress(group, interfaceIndex);
    }

    void MulticastSender::configureIPv6Interface(unsigned int interfaceIndex)
    {
        if (setsockopt(socket_.fd(), IPPROTO_IPV6, IPV6_MULTICAST_IF,
                       &interfaceIndex, sizeof(interfaceIndex)) < 0)
            throw std::runtime_error("Не удалось установить исходящий multicast-интерфейс (IPv6)");
    }

    void MulticastSender::buildDestAddress(const MulticastAddress &group, unsigned int interfaceIndex)
    {
        std::memset(&dest_, 0, sizeof(dest_));

        if (group.isIPv4())
        {
            sockaddr_in dest4{};
            dest4.sin_family = AF_INET;
            dest4.sin_port = htons(config::PORT);
            if (inet_pton(AF_INET, group.text().c_str(), &dest4.sin_addr) != 1)
                throw std::runtime_error("Не удалось разобрать IPv4-адрес группы");
            std::memcpy(&dest_, &dest4, sizeof(dest4));
            destLen_ = sizeof(dest4);
        }
        else
        {
            sockaddr_in6 dest6{};
            dest6.sin6_family = AF_INET6;
            dest6.sin6_port = htons(config::PORT);
            if (inet_pton(AF_INET6, group.text().c_str(), &dest6.sin6_addr) != 1)
                throw std::runtime_error("Не удалось разобрать IPv6-адрес группы");
            dest6.sin6_scope_id = interfaceIndex;
            std::memcpy(&dest_, &dest6, sizeof(dest6));
            destLen_ = sizeof(dest6);
        }
    }

    void MulticastSender::send(const std::string &message) const
    {
        ssize_t sent = sendto(socket_.fd(), message.data(), message.size(), 0,
                              reinterpret_cast<const sockaddr *>(&dest_), destLen_);
        if (sent < 0)
            throw std::runtime_error("Ошибка отправки сообщения (sendto)");
    }
}
