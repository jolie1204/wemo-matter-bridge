#pragma once

#include <cstdint>
#include <string>

namespace wemo_bridge {

struct WemoDevice
{
    std::string udn;
    std::string friendly_name;
    bool supports_level = false;
    uint8_t onoff = 0;
    uint8_t level_percent = 0; // 0..100
};

} // namespace wemo_bridge
