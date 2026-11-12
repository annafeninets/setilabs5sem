#include "epoll.hpp"

#include <cerrno>
#include <system_error>

namespace {
constexpr int kMaxEvents = 256;

[[noreturn]] void fail(const char* what) {
    throw std::system_error(errno, std::generic_category(), what);
}
}

Epoll::Epoll() : fd_(epoll_create1(EPOLL_CLOEXEC)) {
    if (!fd_.valid()) fail("epoll_create1");
}

void Epoll::add(int fd, uint32_t events, EventHandler* handler) {
    epoll_event ev{};
    ev.events = events;
    ev.data.ptr = handler;
    epoll_ctl(fd_.get(), EPOLL_CTL_ADD, fd, &ev);
}

void Epoll::modify(int fd, uint32_t events, EventHandler* handler) {
    epoll_event ev{};
    ev.events = events;
    ev.data.ptr = handler;
    epoll_ctl(fd_.get(), EPOLL_CTL_MOD, fd, &ev);
}

void Epoll::remove(int fd) { epoll_ctl(fd_.get(), EPOLL_CTL_DEL, fd, nullptr); }

void Epoll::poll(int timeoutMs) {
    epoll_event evs[kMaxEvents];
    int n = epoll_wait(fd_.get(), evs, kMaxEvents, timeoutMs);
    if (n < 0) {
        if (errno == EINTR) return;
        fail("epoll_wait");
    }
    for (int i = 0; i < n; ++i) {
        static_cast<EventHandler*>(evs[i].data.ptr)->onEvent(evs[i].events);
    }
}

void Watch::set(int fd, uint32_t mask) {
    if (registered_ && fd != fd_) remove();
    fd_ = fd;
    if (mask == 0) {
        remove();
    } else if (!registered_) {
        epoll_.add(fd, mask, &handler_);
        registered_ = true;
        mask_ = mask;
    } else if (mask != mask_) {
        epoll_.modify(fd, mask, &handler_);
        mask_ = mask;
    }
}

void Watch::remove() {
    if (registered_) epoll_.remove(fd_);
    registered_ = false;
    mask_ = 0;
}