#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace {
    constexpr size_t kCompactThresholdDiv = 4;
}

class Buffer {
public:
    explicit Buffer(size_t capacity) : cap_(capacity) {}

    size_t size() const { return tail_ - head_; }
    bool empty() const { return head_ == tail_; }
    size_t space() const { return cap_ - size(); }

    const uint8_t* readPtr() const { return data_.data() + head_; }

    void consume(size_t n) {
        head_ += n;
        if (head_ == tail_) head_ = tail_ = 0;
    }

    uint8_t* writePtr(size_t& n) {
        ensure();
        if (head_ > 0 && cap_ - tail_ < cap_ / kCompactThresholdDiv) compact();
        n = cap_ - tail_;
        return data_.data() + tail_;
    }

    void commit(size_t n) { tail_ += n; }

    bool append(const uint8_t* p, size_t n) {
        if (n > space()) return false;
        ensure();
        if (cap_ - tail_ < n) compact();
        std::memcpy(data_.data() + tail_, p, n);
        tail_ += n;
        return true;
    }

    void clear() { head_ = tail_ = 0; }

private:
    void ensure() {
        if (data_.empty()) data_.resize(cap_);
    }
    void compact() {
        std::memmove(data_.data(), data_.data() + head_, size());
        tail_ -= head_;
        head_ = 0;
    }

    std::vector<uint8_t> data_;
    size_t cap_;
    size_t head_ = 0, tail_ = 0;
};