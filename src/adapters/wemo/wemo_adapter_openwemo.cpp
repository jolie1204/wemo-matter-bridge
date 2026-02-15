#include "wemo_bridge/wemo_adapter_openwemo.h"

namespace wemo_bridge {

WemoAdapterOpenWemo::WemoAdapterOpenWemo(std::string engine_socket) : mEngineSocket(std::move(engine_socket)) {}

std::vector<WemoDevice> WemoAdapterOpenWemo::Discover()
{
    // TODO: wire to openwemo-bridge-core discovery API.
    return {};
}

bool WemoAdapterOpenWemo::SetOnOff(const std::string &, bool)
{
    // TODO: wire to openwemo-bridge-core OnOff API.
    return false;
}

bool WemoAdapterOpenWemo::SetLevelPercent(const std::string &, uint8_t)
{
    // TODO: wire to openwemo-bridge-core dimmer API.
    return false;
}

} // namespace wemo_bridge
