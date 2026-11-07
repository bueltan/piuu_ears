#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
task_test_dir=$(mktemp -d /tmp/ears-bridge-tests.XXXXXX)
trap 'rm -rf "$task_test_dir"' EXIT

echo "=== Running Bridge C++ Unit Tests ==="
g++ -std=c++11 -Wall -Wextra -I test/bridge/mocks -I src test/bridge/test_bridge.cpp -o "$task_test_dir/test_bridge"
"$task_test_dir/test_bridge"

echo "=== Running Bridge Python Host Tests ==="
python3 test/bridge/test_host.py

echo "=== Running Bridge Integrated Tests ==="
g++ -std=c++11 -Wall -Wextra -I test/bridge/mocks -I src test/bridge/test_integrated.cpp -o "$task_test_dir/test_integrated"
"$task_test_dir/test_integrated"
