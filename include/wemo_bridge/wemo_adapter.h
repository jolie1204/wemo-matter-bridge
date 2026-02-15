#pragma once

#include <string>
#include <vector>

#include "wemo_bridge/wemo_device.h"

namespace wemo_bridge {

class WemoAdapter
{
public:
    virtual ~WemoAdapter() = default;

    virtual std::vector<WemoDevice> Discover() = 0;
    virtual bool SetOnOff(const std::string & udn, bool on) = 0;
    virtual bool SetLevelPercent(const std::string & udn, uint8_t percent) = 0;
};

} // namespace wemo_bridge
