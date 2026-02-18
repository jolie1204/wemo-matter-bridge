# WeMo Survival HOWTO (Post-Belkin Support)

This guide is for users who still have working WeMo devices on LAN but need a
local Matter bridge so those devices remain usable from modern home apps.

## 1. What You Are Building
You are running two local services:
1. `wemo_ctrl` (LAN WeMo discovery/control engine)
2. `wemo-bridge-app` (Matter bridge exposing WeMo devices to controllers)

`wemo-bridge-app` talks to `wemo_ctrl` over local IPC, then publishes bridged
Matter endpoints for apps like Google Home.

## 2. Recommended Baseline
Use a pinned CHIP baseline rather than bleeding edge.

Recommended:
1. `connectedhomeip` on `v1.5.0.1` or a pinned commit on a stable release branch
2. `wemo-matter-bridge` current `master`
3. `openwemo-bridge-core` matching bridge integration expectations

If you change CHIP versions, expect recommissioning and re-validation.

## 3. Workspace Layout
Place repos side-by-side:

```text
<workspace>/
  connectedhomeip/
  openwemo-bridge-core/
  wemo-matter-bridge/
```

`wemo-matter-bridge/matter-bridge-app` expects `../../connectedhomeip`.

## 4. Build Steps

### 4.1 Build `wemo-bridge-app` (Matter bridge)
```bash
cd wemo-matter-bridge/matter-bridge-app
./build_wemo_bridge.sh
```

Expected output:
`wemo-matter-bridge/matter-bridge-app/out/ethernet/wemo-bridge-app`

Notes:
1. Build script may apply bridge-managed CHIP patches from
   `matter-bridge-app/patches/connectedhomeip/`.
2. If a patch is not applicable to your pinned CHIP revision, script may warn
   and continue.

### 4.2 Build `wemo_ctrl`
Build from `openwemo-bridge-core` using its documented build steps.

## 5. Deploy and Start

If your environment includes a helper script (for example `bin/bridge_stack.sh`):
```bash
<workspace>/bin/bridge_stack.sh stop
cp wemo-matter-bridge/matter-bridge-app/out/ethernet/wemo-bridge-app <workspace>/bin/wemo-bridge-app
<workspace>/bin/bridge_stack.sh start
<workspace>/bin/bridge_stack.sh status
```

Verify:
1. `wemo_ctrl` is running
2. `wemo-bridge-app` is running
3. Bridge log path is known (typically `var/log/wemo_bridge.log`)

## 6. Commission to Google Home
1. Put bridge in commissionable mode (normal startup for bridge-app on fresh fabric).
2. Add Matter device in Google Home.
3. Wait until all endpoints appear.
4. Validate each important device:
   - On/off for switches
   - Level control for dimmers

## 7. Post-Commission Validation
Use logs to confirm endpoint binding:

```bash
rg -n "WeMo bind|Added device|Device\\[" var/log/wemo_bridge.log | tail -n 200
```

You should see entries like:
1. `WeMo bind (dimmable): Kitchen <- uuid:Dimmer-...`
2. `Added device Kitchen to dynamic endpoint ...`

## 8. Troubleshooting by Symptom

### Symptom A: Devices show offline
1. Confirm both daemons are running.
2. Confirm LAN reachability and multicast/UPnP availability.
3. Check startup log for discovery/bind lines.
4. Restart stack after confirming network interface is correct.

### Symptom B: Toggling one tile controls another device
1. Check bridge log to confirm both devices are discovered with correct names.
2. Recommission bridge in controller app.
3. If still wrong, controller may have stale endpoint identity cache.
4. Keep endpoint assignment deterministic (UDN sorting) and avoid changing
   identity model frequently between releases.

### Symptom C: Dimmers do not appear dimmable
1. Confirm `supports_level` is true for dimmer device in discovery.
2. Confirm Level Control cluster is present on dynamic endpoint.
3. Remove/re-add bridge in controller if cluster capability changed after
   previous commissioning.

### Symptom D: Off action changes dim level
1. Verify bridge logs for synthetic min-level suppression.
2. Ensure no stale binary is running after rebuild/deploy.

## 9. Upgrade Strategy (Safe)
For each upgrade:
1. Pin target CHIP SHA.
2. Build and deploy in maintenance window.
3. Validate with a smoke sequence:
   - discover
   - one on/off per room
   - one dimmer level change
   - restart stack and verify state behavior
4. Only then mark release as known-good.

## 10. Rollback Strategy
Keep previous known-good binaries and SHAs.

Rollback flow:
1. Stop stack.
2. Restore previous `wemo-bridge-app` binary.
3. Restore previous dependency pin if required.
4. Restart stack.
5. Confirm old behavior returns before further changes.

## 11. Support Bundle for Bug Reports
When filing issues, include:
1. `wemo-matter-bridge` commit SHA
2. `connectedhomeip` SHA/tag
3. Controller app/version
4. Device model(s)
5. Relevant excerpts from:
   - `var/log/wemo_bridge.log`
   - `var/log/wemo_ctrl.log`
6. Exact timestamp and action performed

This makes mapping/control bugs diagnosable quickly.
