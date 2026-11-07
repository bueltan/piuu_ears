#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

echo "=========================================="
echo " [TEST SUITE] C3_EARS Host Unit Tests"
echo "=========================================="

echo -e "\n[1/3] Executing Audio Routing & Link Tests..."
bash test/run_audio_routing_tests.sh

echo -e "\n[2/3] Executing Bridge Tests..."
bash test/run_bridge_tests.sh

echo -e "\n[3/3] Executing Diagnostics & Hardware Module Tests..."
bash test/run_diagnostics_tests.sh

echo -e "\n=========================================="
echo " [SUCCESS] All C3_EARS host tests passed."
echo "=========================================="
