#include "wemo_bridge/wemo_adapter.h"

namespace wemo_bridge {

class WemoAdapterStub final : public WemoAdapter
{
public:
    std::vector<WemoDevice> Discover() override { return {}; }

    bool SetOnOff(const std::string &, bool) override { return false; }

    bool SetLevelPercent(const std::string &, uint8_t) override { return false; }
};

} // namespace wemo_bridge
