#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
task_test_dir=$(mktemp -d /tmp/ears-diag-tests.XXXXXX)
trap 'rm -rf "$task_test_dir"' EXIT

echo "=== Running EYES USB Command Tests ==="
g++ -std=c++11 -Wall -Wextra -Werror -I src test/eyes_usb/test_eyes_usb.cpp -o "$task_test_dir/test_eyes_usb"
"$task_test_dir/test_eyes_usb"

echo "=== Running UART Diagnostics Tests ==="
g++ -std=c++11 -pthread -I test/link_host -I src test/link_host/test_diagnostics.cpp src/uart_diagnostics.cpp -o "$task_test_dir/test_diagnostics"
"$task_test_dir/test_diagnostics"

echo "=== Running Microphone PCM Tests ==="
g++ -std=c++11 -pthread -I test/link_host -I src test/link_host/test_microphone_pcm.cpp -o "$task_test_dir/test_mic_pcm"
"$task_test_dir/test_mic_pcm"

echo "=== Running Microphone Startup Warmup Tests ==="
g++ -std=c++11 -pthread -I test/link_host -I src test/link_host/test_microphone_startup.cpp -o "$task_test_dir/test_mic_startup"
"$task_test_dir/test_mic_startup"
