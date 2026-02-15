#pragma once

#include <string>

#include "wemo_bridge/wemo_adapter.h"

namespace wemo_bridge {

class WemoAdapterOpenWemo final : public WemoAdapter
{
public:
    explicit WemoAdapterOpenWemo(std::string engine_socket);

    std::vector<WemoDevice> Discover() override;
    bool SetOnOff(const std::string & udn, bool on) override;
    bool SetLevelPercent(const std::string & udn, uint8_t percent) override;

private:
    std::string mEngineSocket;
};

} // namespace wemo_bridge
