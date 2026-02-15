#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

if [[ ! -d connectedhomeip ]]; then
  echo "missing sibling path: ../../connectedhomeip" >&2
  exit 1
fi

CHIP_ROOT="$(cd ../../connectedhomeip && pwd -P)"

HOME=/tmp /bin/bash "${CHIP_ROOT}/scripts/examples/gn_build_example.sh" . out/ethernet

echo "Built: $(pwd)/out/ethernet/wemo-bridge-app"
