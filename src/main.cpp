#include <iostream>

#include "wemo_bridge/endpoint_registry.h"
#include "wemo_bridge/wemo_adapter_openwemo.h"

int main()
{
    wemo_bridge::EndpointRegistry registry("./var/endpoint-map.sqlite3");
    const auto endpoint_id = registry.GetOrAssign("uuid:bootstrap-device");

    wemo_bridge::WemoAdapterOpenWemo adapter("/tmp/wemo_engine.sock");
    const auto devices = adapter.Discover();

    if (!endpoint_id.has_value())
    {
        std::cerr << "failed to assign endpoint id" << std::endl;
        return 1;
    }

    std::cout << "wemo-bridge-app initialized, bootstrap endpoint=" << endpoint_id.value()
              << ", discovered_devices=" << devices.size() << std::endl;
    return 0;
}
