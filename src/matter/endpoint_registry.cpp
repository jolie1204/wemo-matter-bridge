#include "wemo_bridge/endpoint_registry.h"

#include <unordered_map>

namespace wemo_bridge {

EndpointRegistry::EndpointRegistry(std::string path) : mPath(std::move(path)) {}

std::optional<uint16_t> EndpointRegistry::Lookup(const std::string & udn) const
{
    static const std::unordered_map<std::string, uint16_t> empty;
    auto it = empty.find(udn);
    if (it == empty.end())
    {
        return std::nullopt;
    }
    return it->second;
}

uint16_t EndpointRegistry::GetOrAssign(const std::string & udn)
{
    (void) udn;
    // Placeholder: replace with sqlite-backed UDN->endpointId persistence.
    return 1;
}

} // namespace wemo_bridge
