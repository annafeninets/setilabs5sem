#include "speed_tracker.hpp"
#include "../common/display_units.hpp"
#include "../common/logger.hpp"

#include <iomanip>
#include <sstream>

using namespace std::chrono_literals;

SpeedTracker::SpeedTracker(std::string clientLabel, const std::atomic<uint64_t>& receivedBytes,
                            uint64_t totalBytes)
    : clientLabel_(std::move(clientLabel)), receivedBytes_(receivedBytes), totalBytes_(totalBytes) {}

SpeedTracker::~SpeedTracker() { stop(); }

void SpeedTracker::start() {
    sessionStart_ = std::chrono::steady_clock::now();
    thread_ = std::thread(&SpeedTracker::run, this);
}

void SpeedTracker::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopped_) return;
        stopped_ = true;
        done_ = true;
    }
    cv_.notify_all();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void SpeedTracker::run() {
    auto intervalStart = sessionStart_;
    uint64_t bytesAtIntervalStart = 0;

    std::unique_lock<std::mutex> lock(mutex_);
    while (true) {
        bool finishedEarly = cv_.wait_for(lock, 3s, [this] { return done_; });
        auto now = std::chrono::steady_clock::now();
        uint64_t currentBytes = receivedBytes_.load();

        if (finishedEarly) {
            if (!reportedAny_) {
                lock.unlock();
                printReport(intervalStart, now, bytesAtIntervalStart, currentBytes);
            }
            return;
        }

        lock.unlock();
        printReport(intervalStart, now, bytesAtIntervalStart, currentBytes);
        reportedAny_ = true;
        intervalStart = now;
        bytesAtIntervalStart = currentBytes;
        lock.lock();
    }
}

void SpeedTracker::printReport(std::chrono::steady_clock::time_point intervalStart,
                                std::chrono::steady_clock::time_point now,
                                uint64_t bytesAtIntervalStart, uint64_t currentBytes) const {
    double intervalSeconds = std::chrono::duration<double>(now - intervalStart).count();
    double totalSeconds = std::chrono::duration<double>(now - sessionStart_).count();

    double instantSpeed = intervalSeconds > 0
                               ? static_cast<double>(currentBytes - bytesAtIntervalStart) / intervalSeconds
                               : 0.0;
    double avgSpeed = totalSeconds > 0 ? static_cast<double>(currentBytes) / totalSeconds : 0.0;

    double currentMB = static_cast<double>(currentBytes) / kBytesPerMegabyte;
    double totalMB = static_cast<double>(totalBytes_) / kBytesPerMegabyte;
    double instantSpeedMBs = instantSpeed / kBytesPerMegabyte;
    double avgSpeedMBs = avgSpeed / kBytesPerMegabyte;

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "[" << clientLabel_ << "] " << currentMB << "/" << totalMB << " MB | "
        << "instant: " << instantSpeedMBs << " MB/s | average: " << avgSpeedMBs << " MB/s";
    Logger::log(oss.str());
}
