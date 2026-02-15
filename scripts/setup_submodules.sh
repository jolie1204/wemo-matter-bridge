#!/usr/bin/env bash
set -euo pipefail

if [[ ! -d .git ]]; then
  echo "Run this script from the repository root." >&2
  exit 1
fi

if [[ -d third_party/connectedhomeip ]]; then
  echo "third_party/connectedhomeip already exists"
else
  git submodule add git@github.com:puffie/connectedhomeip.git third_party/connectedhomeip
fi

echo "Submodules configured. Pin to a specific commit before release."
