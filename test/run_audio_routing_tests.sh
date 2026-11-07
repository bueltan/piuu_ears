#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
task_test_dir=$(mktemp -d /tmp/ears-audio-routing.XXXXXX)
trap 'rm -rf "$task_test_dir"' EXIT

g++ -std=c++11 -pthread -Wall -Wextra -I test/audio_pause/mocks -I src test/audio_pause/test_pause.cpp src/ears_state.cpp -o "$task_test_dir/audio_pause"
"$task_test_dir/audio_pause"
g++ -std=c++11 -pthread -Wall -Wextra -I test/link_host -I src test/link_host/test_link.cpp src/core_link.cpp src/core_log_relay.cpp src/wake_link.cpp src/audio_protocol.cpp src/ears_state.cpp -o "$task_test_dir/link"
"$task_test_dir/link"
g++ -std=c++11 -pthread -Wall -Wextra -DEARS_LOGS_ENABLED=0 -DEARS_MIC_DIAGNOSTICS=0 -DEARS_UART_DIAGNOSTICS=0 -I test/audio_pause/mocks -I src test/audio_pause/test_pause.cpp src/ears_state.cpp -o "$task_test_dir/audio_pause_quiet"
"$task_test_dir/audio_pause_quiet"
g++ -std=c++11 -pthread -Wall -Wextra -DEARS_LOGS_ENABLED=0 -DEARS_MIC_DIAGNOSTICS=0 -DEARS_UART_DIAGNOSTICS=0 -I test/link_host -I src test/link_host/test_link.cpp src/core_link.cpp src/core_log_relay.cpp src/wake_link.cpp src/audio_protocol.cpp src/ears_state.cpp -o "$task_test_dir/link_quiet"
"$task_test_dir/link_quiet"
