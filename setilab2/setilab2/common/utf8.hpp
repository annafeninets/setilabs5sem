#pragma once

#include <string>

bool isValidUtf8(const std::string& s) noexcept;

std::string truncateUtf8(const std::string& s, size_t maxBytes);
