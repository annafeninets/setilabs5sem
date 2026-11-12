#pragma once

#include <sys/epoll.h>

#include <cstdint>

#include "fd.hpp"

class EventHandler {
public:
    virtual ~EventHandler() = default;
    virtual void onEvent(uint32_t events) = 0;
};

class Epoll {
public:
    Epoll();

    void add(int fd, uint32_t events, EventHandler* handler);
    void modify(int fd, uint32_t events, EventHandler* handler);
    void remove(int fd);

    void poll(int timeoutMs);

private:
    FileDescriptor fd_;
};

class Watch {
public:
    Watch(Epoll& epoll, EventHandler& handler) : epoll_(epoll), handler_(handler) {}
    ~Watch() { remove(); }
    Watch(const Watch&) = delete;
    Watch& operator=(const Watch&) = delete;

    void set(int fd, uint32_t mask);
    void remove();

private:
    Epoll& epoll_;
    EventHandler& handler_;
    int fd_ = -1;
    bool registered_ = false;
    uint32_t mask_ = 0;
};