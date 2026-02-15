# wemo-matter-bridge

Matter bridge application for exposing legacy WeMo devices as Matter endpoints.

## Scope
- Bridge-only architecture (no firmware flashing or device bootloader changes).
- Uses WeMo UDN as canonical identity.
- Persists `UDN -> endpointId` for endpoint stability.

## Repository layout
- `src/`: app sources and adapters
- `include/`: public headers
- `config/`: runtime config examples
- `.github/workflows/`: CI skeleton

## Dependency model (recommended)
- `connectedhomeip` pinned by commit via submodule at `third_party/connectedhomeip`
- `openwemo-bridge-core` consumed as package/submodule

## Quick start
```bash
# 1) initialize submodules (after adding them)
git submodule update --init --recursive

# 2) configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# 3) build
cmake --build build -j
```

## Notes
- Keep CHIP-core patches minimal and upstreamable.
- Keep bridge-specific logic in this repo.
