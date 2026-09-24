#include "Multicast sender.h"
#include "Constants.h"

#include <arpa/inet.h>
#include <cstring>
#include <net/if.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

MulticastSender::MulticastSender(const MulticastAddress &group)
{
    socketFd_ = socket(group.family(), SOCK_DGRAM, 0);
    if (socketFd_ < 0)
        throw std::runtime_error("Не удалось создать отправляющий сокет");

    buildDestAddress(group);
}

MulticastSender::~MulticastSender()
{
    if (socketFd_ >= 0)
        close(socketFd_);
}

void MulticastSender::buildDestAddress(const MulticastAddress &group)
{
    memset(&dest_, 0, sizeof(dest_));

    if (group.isIPv4())
    {
        sockaddr_in dest4{};
        dest4.sin_family = AF_INET;
        dest4.sin_port = htons(config::PORT);
        dest4.sin_addr.s_addr = inet_addr(group.text().c_str());
        memcpy(&dest_, &dest4, sizeof(dest4));
        destLen_ = sizeof(dest4);
    }
    else
    {
        sockaddr_in6 dest6{};
        dest6.sin6_family = AF_INET6;
        dest6.sin6_port = htons(config::PORT);
        inet_pton(AF_INET6, group.text().c_str(), &dest6.sin6_addr);
        dest6.sin6_scope_id = if_nametoindex(config::NETWORK_INTERFACE);
        memcpy(&dest_, &dest6, sizeof(dest6));
        destLen_ = sizeof(dest6);
    }
}

void MulticastSender::send(const std::string &message) const
{
    ssize_t sent = sendto(socketFd_, message.data(), message.size(), 0,
                           reinterpret_cast<const sockaddr *>(&dest_), destLen_);
    if (sent < 0)
        throw std::runtime_error("Ошибка отправки сообщения (sendto)");
}