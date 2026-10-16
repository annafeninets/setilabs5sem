#pragma once

#include <iostream>
#include <mutex>
#include <sstream>

class Logger {
public:
    template <typename... Args>
    static void log(Args&&... args) {
        std::ostringstream oss;
        (oss << ... << args);
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << oss.str() << std::endl;
    }

private:
    static inline std::mutex mutex_;
};
