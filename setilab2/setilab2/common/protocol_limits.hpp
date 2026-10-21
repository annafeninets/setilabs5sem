#pragma once

#include <cstdint>

inline constexpr uint64_t kMaxFileNameBytes = 4096;

inline constexpr uint64_t kMaxFileSizeBytes = 1ULL << 40;

inline constexpr int kIoTimeoutSeconds = 30;

inline constexpr const char* kStatusSuccess = "SUCCESS";
inline constexpr const char* kStatusFail = "FAIL";
