#include "config.hpp"

#include "util.hpp"

ApiConfig ApiConfig::load(const std::string& envFile) {
    auto file = loadEnv(envFile);
    ApiConfig c;
    c.graphhopperKey_ = envOr(file, "API_KEY_GRAPHHOPPER");
    c.openweatherKey_ = envOr(file, "API_KEY_OPENWEATHER");
    c.opentripmapKey_ = envOr(file, "API_KEY_OPENTRIPMAP");
    return c;
}

bool ApiConfig::hasAllKeys() const {
    return !graphhopperKey_.empty() && !openweatherKey_.empty() && !opentripmapKey_.empty();
}