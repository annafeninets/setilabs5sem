#pragma once

#include "interfaces.hpp"

class TextReportRenderer final : public IReportRenderer {
public:
    std::string render(const Report& report) const override;
};
