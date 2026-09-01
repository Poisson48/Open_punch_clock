#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
export QT_QPA_PLATFORM=offscreen
timeout 30 ./build/src/openpunchclock || true
echo "Smoke OK"
