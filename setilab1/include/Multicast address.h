#pragma once

#include <string>
#include <sys/socket.h>

namespace setilab1
{
    class MulticastAddress
    {
    public:
        explicit MulticastAddress(const std::string &text);

        const std::string &text() const { return text_; }
        int family() const { return family_; }
        bool isIPv4() const { return family_ == AF_INET; }
        bool isIPv6() const { return family_ == AF_INET6; }

    private:
        std::string text_;
        int family_;

        static int detectFamily(const std::string &text);
        static bool isMulticast(const std::string &text, int family);
    };
}