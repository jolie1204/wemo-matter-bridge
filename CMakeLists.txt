cmake_minimum_required(VERSION 3.20)
project(wemo_matter_bridge LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WEMO_BRIDGE_USE_OPENWEMO_CORE "Enable openwemo-bridge-core integration" ON)

add_executable(wemo-bridge-app
    src/main.cpp
    src/matter/endpoint_registry.cpp
    src/adapters/wemo/wemo_adapter_stub.cpp
    src/adapters/wemo/wemo_adapter_openwemo.cpp
)

target_include_directories(wemo-bridge-app
    PRIVATE
        include
)

find_package(SQLite3 REQUIRED)
target_link_libraries(wemo-bridge-app PRIVATE SQLite::SQLite3)

# Integration points:
# - linked CHIP targets from third_party/connectedhomeip
# - linked openwemo-bridge-core targets
# Keep this file small and explicit; add linkage when dependency wiring is finalized.
