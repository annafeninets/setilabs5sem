#pragma once

namespace setilab1
{
    class UdpSocket
    {
    public:
        explicit UdpSocket(int family);
        ~UdpSocket();

        UdpSocket(const UdpSocket &) = delete;
        UdpSocket &operator=(const UdpSocket &) = delete;

        UdpSocket(UdpSocket &&other) noexcept;
        UdpSocket &operator=(UdpSocket &&other) noexcept;

        int fd() const { return fd_; }

    private:
        int fd_;
    };
}
