#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

class SpeedTracker {
public:
    SpeedTracker(std::string clientLabel, const std::atomic<uint64_t>& receivedBytes,
                 uint64_t totalBytes);
    ~SpeedTracker();

    SpeedTracker(const SpeedTracker&) = delete;
    SpeedTracker& operator=(const SpeedTracker&) = delete;

    void start();
    void stop();

private:
    void run();
    void printReport(std::chrono::steady_clock::time_point intervalStart,
                      std::chrono::steady_clock::time_point now, uint64_t bytesAtIntervalStart,
                      uint64_t currentBytes) const;

    std::string clientLabel_;
    const std::atomic<uint64_t>& receivedBytes_;
    uint64_t totalBytes_;

    std::thread thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool done_ = false;
    bool stopped_ = false;
    bool reportedAny_ = false;
    std::chrono::steady_clock::time_point sessionStart_;
};
