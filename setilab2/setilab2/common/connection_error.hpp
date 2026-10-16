#pragma once

#include <stdexcept>
#include <string>

class ConnectionError : public std::runtime_error {
public:
    explicit ConnectionError(const std::string& what) : std::runtime_error(what) {}
};
