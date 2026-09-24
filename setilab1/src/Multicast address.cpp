#include "Multicast address.h"

#include <arpa/inet.h>
#include <stdexcept>

MulticastAddress::MulticastAddress(const std::string &text)
    : text_(text), family_(detectFamily(text))
{
    if (family_ == -1)
        throw std::runtime_error("Некорректный IP-адрес: " + text_);

    if (!isMulticast(text_, family_))
        throw std::runtime_error("Адрес должен принадлежать multicast-диапазону: " + text_);
}

int MulticastAddress::detectFamily(const std::string &text)
{
    in_addr addr4{};
    if (inet_pton(AF_INET, text.c_str(), &addr4) == 1)
        return AF_INET;

    in6_addr addr6{};
    if (inet_pton(AF_INET6, text.c_str(), &addr6) == 1)
        return AF_INET6;

    return -1;
}

bool MulticastAddress::isMulticast(const std::string &text, int family)
{
    if (family == AF_INET)
    {
        in_addr addr{};
        inet_pton(AF_INET, text.c_str(), &addr);
        uint32_t ipHostOrder = ntohl(addr.s_addr);
        return ipHostOrder >= 0xE0000000u && ipHostOrder <= 0xEFFFFFFFu;
    }

    in6_addr addr6{};
    inet_pton(AF_INET6, text.c_str(), &addr6);
    return addr6.s6_addr[0] == 0xFF;
}