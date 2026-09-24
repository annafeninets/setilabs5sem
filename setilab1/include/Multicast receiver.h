#pragma once

#include "Multicast address.h"

#include <optional>
#include <string>
#include <sys/socket.h>

class MulticastReceiver
{
public:
    explicit MulticastReceiver(const MulticastAddress &group);
    ~MulticastReceiver();

    MulticastReceiver(const MulticastReceiver &) = delete;
    MulticastReceiver &operator=(const MulticastReceiver &) = delete;

    std::optional<std::string> receive(const std::string &expectedMessage) const;

private:
    int socketFd_;

    void joinGroup(const MulticastAddress &group);
    static std::string senderToString(const sockaddr_storage &sender);
};