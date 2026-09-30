#include "udp_socket.h"

#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace setilab1
{
    UdpSocket::UdpSocket(int family) : fd_(socket(family, SOCK_DGRAM, 0))
    {
        if (fd_ < 0)
            throw std::runtime_error("Не удалось создать UDP-сокет");
    }

    UdpSocket::~UdpSocket()
    {
        if (fd_ >= 0)
            close(fd_);
    }

    UdpSocket::UdpSocket(UdpSocket &&other) noexcept : fd_(other.fd_)
    {
        other.fd_ = -1;
    }

    UdpSocket &UdpSocket::operator=(UdpSocket &&other) noexcept
    {
        if (this != &other)
        {
            if (fd_ >= 0)
                close(fd_);
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }
}
