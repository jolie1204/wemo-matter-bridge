#include "wemo_bridge/wemo_adapter_openwemo.h"

#include <sqlite3.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#if HAVE_OPENWEMO_ENGINE
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif
extern "C" {
#define export we_export_param
#include "wemo_engine.h"
#undef export
}
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#endif

namespace wemo_bridge {

namespace {

constexpr int kCapBinary = 1;
constexpr int kCapLevel  = 2;
constexpr int kTypeDimmer = 4;

std::string DefaultStateDir()
{
    if (geteuid() == 0)
    {
        return "/var/lib/wemo-matter";
    }

    const char * home = std::getenv("HOME");
    if (home != nullptr && home[0] != '\0')
    {
        return std::string(home) + "/.local/state/wemo-matter";
    }

    return "/var/tmp/wemo-matter";
}

std::string ResolveDeviceDbPath()
{
    const char * override = std::getenv("WEMO_DEVICE_DB_PATH");
    if (override != nullptr && override[0] != '\0')
    {
        return override;
    }

    return DefaultStateDir() + "/wemo_device.db";
}

std::string ResolveStateDbPath()
{
    const char * override = std::getenv("WEMO_STATE_DB_PATH");
    if (override != nullptr && override[0] != '\0')
    {
        return override;
    }

    return DefaultStateDir() + "/wemo_state.db";
}

std::optional<int> QueryWemoIdByUdn(const std::string &device_db_path, const std::string & udn)
{
    sqlite3 * db = nullptr;
    if (sqlite3_open_v2(device_db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
    {
        if (db != nullptr)
        {
            sqlite3_close(db);
        }
        return std::nullopt;
    }

    sqlite3_stmt * stmt = nullptr;
    const char * sql    = "SELECT wemo_id FROM wemo_device WHERE UDN = ?1;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        sqlite3_close(db);
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, udn.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<int> result = std::nullopt;
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        result = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return result;
}

void PopulateState(const std::string & state_db_path, std::vector<WemoDevice> & devices)
{
    sqlite3 * db = nullptr;
    if (sqlite3_open_v2(state_db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
    {
        if (db != nullptr)
        {
            sqlite3_close(db);
        }
        return;
    }

    sqlite3_stmt * online_stmt = nullptr;
    if (sqlite3_prepare_v2(db, "SELECT is_online FROM state WHERE wemo_id = ?1;", -1, &online_stmt, nullptr) != SQLITE_OK)
    {
        sqlite3_close(db);
        return;
    }

    sqlite3_stmt * cap_stmt = nullptr;
    if (sqlite3_prepare_v2(db,
                           "SELECT value FROM state_capability WHERE wemo_id = ?1 AND cap = ?2;",
                           -1,
                           &cap_stmt,
                           nullptr) != SQLITE_OK)
    {
        sqlite3_finalize(online_stmt);
        sqlite3_close(db);
        return;
    }

    for (auto & device : devices)
    {
        sqlite3_reset(online_stmt);
        sqlite3_clear_bindings(online_stmt);
        sqlite3_bind_int(online_stmt, 1, device.wemo_id);
        if (sqlite3_step(online_stmt) == SQLITE_ROW)
        {
            device.is_online = (sqlite3_column_int(online_stmt, 0) != 0);
        }

        sqlite3_reset(cap_stmt);
        sqlite3_clear_bindings(cap_stmt);
        sqlite3_bind_int(cap_stmt, 1, device.wemo_id);
        sqlite3_bind_int(cap_stmt, 2, kCapBinary);
        if (sqlite3_step(cap_stmt) == SQLITE_ROW)
        {
            device.onoff = static_cast<uint8_t>(sqlite3_column_int(cap_stmt, 0) ? 1 : 0);
        }

        sqlite3_reset(cap_stmt);
        sqlite3_clear_bindings(cap_stmt);
        sqlite3_bind_int(cap_stmt, 1, device.wemo_id);
        sqlite3_bind_int(cap_stmt, 2, kCapLevel);
        if (sqlite3_step(cap_stmt) == SQLITE_ROW)
        {
            int level = sqlite3_column_int(cap_stmt, 0);
            level = std::clamp(level, 0, 100);
            device.level_percent = static_cast<uint8_t>(level);
            device.supports_level = true;
        }
    }

    sqlite3_finalize(cap_stmt);
    sqlite3_finalize(online_stmt);
    sqlite3_close(db);
}

std::vector<WemoDevice> QueryDevicesFromDb(const std::string & device_db_path, const std::string & state_db_path)
{
    std::vector<WemoDevice> devices;

    sqlite3 * db = nullptr;
    if (sqlite3_open_v2(device_db_path.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
    {
        if (db != nullptr)
        {
            sqlite3_close(db);
        }
        return devices;
    }

    sqlite3_stmt * stmt = nullptr;
    const char * sql = "SELECT wemo_id, UDN, device_type, friendly_name FROM wemo_device ORDER BY wemo_id;";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    {
        sqlite3_close(db);
        return devices;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        WemoDevice device;
        device.wemo_id = sqlite3_column_int(stmt, 0);

        const unsigned char * udn_text = sqlite3_column_text(stmt, 1);
        if (udn_text != nullptr)
        {
            device.udn = reinterpret_cast<const char *>(udn_text);
        }

        const int device_type = sqlite3_column_int(stmt, 2);
        device.supports_level = (device_type == kTypeDimmer);

        const unsigned char * name_text = sqlite3_column_text(stmt, 3);
        if (name_text != nullptr)
        {
            device.friendly_name = reinterpret_cast<const char *>(name_text);
        }

        devices.push_back(std::move(device));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    PopulateState(state_db_path, devices);
    return devices;
}

#if HAVE_OPENWEMO_ENGINE
void MaybeConfigureIpcTarget(const std::string & engine_socket)
{
    // Backward-compatible parser: if value looks like "host:port", use it as IPC target.
    const auto pos = engine_socket.rfind(':');
    if (pos == std::string::npos || pos == 0 || pos + 1 >= engine_socket.size())
    {
        return;
    }

    const std::string host = engine_socket.substr(0, pos);
    const std::string port_str = engine_socket.substr(pos + 1);
    const int port = std::atoi(port_str.c_str());
    if (port > 0)
    {
        we_set_ipc_target(host.c_str(), port);
    }
}

bool EnsureEngineInitialized(const std::string & engine_socket)
{
    static std::mutex init_lock;
    static bool target_configured = false;
    static bool initialized = false;

    std::lock_guard<std::mutex> lock(init_lock);
    if (initialized)
    {
        return true;
    }

    if (!target_configured)
    {
        MaybeConfigureIpcTarget(engine_socket);
        target_configured = true;
    }

    initialized = (we_init() != 0);
    if (!initialized)
    {
        std::fprintf(stderr, "wemo_adapter: we_init failed (socket=%s)\n", engine_socket.c_str());
    }

    return initialized;
}

bool SendState(int wemo_id, int state, int level)
{
    constexpr int kCommandConfirmTimeoutMs = 2500;

    struct we_state target {};
    target.state     = state;
    target.level     = level;
    target.is_online = 1;

    const int rc_confirm = we_set_action_confirmed(wemo_id, &target, kCommandConfirmTimeoutMs);
    std::fprintf(stderr, "wemo_adapter: set_confirmed wemo_id=%d state=%d level=%d rc=%d\n", wemo_id, state, level, rc_confirm);
    if (rc_confirm == 0)
    {
        return true;
    }
    const int rc_set = we_set_action(wemo_id, &target);
    std::fprintf(stderr, "wemo_adapter: set_fallback wemo_id=%d state=%d level=%d rc=%d\n", wemo_id, state, level, rc_set);
    return rc_set != 0;
}
#endif

} // namespace

WemoAdapterOpenWemo::WemoAdapterOpenWemo(std::string engine_socket) : mEngineSocket(std::move(engine_socket)) {}

std::vector<WemoDevice> WemoAdapterOpenWemo::Discover()
{
#if HAVE_OPENWEMO_ENGINE
    if (EnsureEngineInitialized(mEngineSocket))
    {
        (void) we_discover(0);
    }
#endif

    const std::string device_db_path = ResolveDeviceDbPath();
    const std::string state_db_path  = ResolveStateDbPath();

    return QueryDevicesFromDb(device_db_path, state_db_path);
}

bool WemoAdapterOpenWemo::SetOnOff(const std::string & udn, bool on)
{
#if !HAVE_OPENWEMO_ENGINE
    (void) udn;
    (void) on;
    return false;
#else
    if (!EnsureEngineInitialized(mEngineSocket))
    {
        return false;
    }

    const std::string device_db_path = ResolveDeviceDbPath();
    auto wemo_id = QueryWemoIdByUdn(device_db_path, udn);
    if (!wemo_id.has_value())
    {
        (void) we_discover(0);
        wemo_id = QueryWemoIdByUdn(device_db_path, udn);
    }
    if (!wemo_id.has_value())
    {
        return false;
    }

    return SendState(wemo_id.value(), on ? 1 : 0, -1);
#endif
}

bool WemoAdapterOpenWemo::SetLevelPercent(const std::string & udn, uint8_t percent)
{
#if !HAVE_OPENWEMO_ENGINE
    (void) udn;
    (void) percent;
    return false;
#else
    if (!EnsureEngineInitialized(mEngineSocket))
    {
        return false;
    }

    const std::string device_db_path = ResolveDeviceDbPath();
    auto wemo_id = QueryWemoIdByUdn(device_db_path, udn);
    if (!wemo_id.has_value())
    {
        (void) we_discover(0);
        wemo_id = QueryWemoIdByUdn(device_db_path, udn);
    }
    if (!wemo_id.has_value())
    {
        return false;
    }

    const int clamped = std::clamp(static_cast<int>(percent), 0, 100);
    const int state   = (clamped > 0) ? 1 : 0;
    return SendState(wemo_id.value(), state, clamped);
#endif
}

} // namespace wemo_bridge
