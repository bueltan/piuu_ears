# Testing, Verification & Regression Test Suite Guide

> **Scope**: Host-based C++ unit tests, Python regression suites, mocks, unified test runners, and automated drift auditing for `C3_EARS`.

---

## 1. Test Architecture & Host Mock Philosophy

All firmware core algorithms, serial protocol parsers, audio distribution states, and maintenance bridges are designed with clean separation from hardware registers. This enables executing unit and regression test suites directly on the Linux host using `g++` (C++11) without flashing physical hardware.

The project maintains mock implementations in:
- `test/audio_pause/mocks/`: HardwareSerial and FreeRTOS mocks for audio engine cooperative parking.
- `test/bridge/mocks/`: Mock Serial and GPIO pins for bridge reset and strap sequences.
- `test/link_host/`: Mock Arduino and FreeRTOS environments for dual-UART packet serialization.

---

## 2. Test Suite Catalog

| Test Module | Source Files | Test Runner Script | Covered Capabilities |
| :--- | :--- | :--- | :--- |
| **Audio Routing & Links** | [`test/audio_pause/test_pause.cpp`](../test/audio_pause/test_pause.cpp)<br>[`test/link_host/test_link.cpp`](../test/link_host/test_link.cpp) | [`test/run_audio_routing_tests.sh`](../test/run_audio_routing_tests.sh) | Cooperative parking, concurrent PCM routing, speaker echo suppression, wire framing, command timeouts, standard and quiet builds. |
| **Bridge Subsystem** | [`test/bridge/test_bridge.cpp`](../test/bridge/test_bridge.cpp)<br>[`test/bridge/test_host.py`](../test/bridge/test_host.py)<br>[`test/bridge/test_integrated.cpp`](../test/bridge/test_integrated.cpp) | [`test/run_bridge_tests.sh`](../test/run_bridge_tests.sh) | In-system programmer strapping, binary fragment proxying, CRC16 validation, virtual socket tunnels, flow control, clean exit. |
| **Diagnostics & Hardware** | [`test/eyes_usb/test_eyes_usb.cpp`](../test/eyes_usb/test_eyes_usb.cpp)<br>[`test/link_host/test_diagnostics.cpp`](../test/link_host/test_diagnostics.cpp)<br>[`test/link_host/test_microphone_pcm.cpp`](../test/link_host/test_microphone_pcm.cpp)<br>[`test/link_host/test_microphone_startup.cpp`](../test/link_host/test_microphone_startup.cpp) | [`test/run_diagnostics_tests.sh`](../test/run_diagnostics_tests.sh) | Developer USB `EYES` parsing, UART timing diagnostics, 24-bit to 16-bit PCM conversion with gain saturation, 1000 ms warmup mute. |

---

## 3. Running Test Suites

### 3.1 Unified Host Test Runner
To execute all host test suites in sequence:

```bash
bash test/run_host_tests.sh
```

### 3.2 Individual Test Runners
To target specific subsystems during development:

```bash
# Run audio routing and link framing tests
bash test/run_audio_routing_tests.sh

# Run bridge in-system programmer tests
bash test/run_bridge_tests.sh

# Run diagnostics and signal processing tests
bash test/run_diagnostics_tests.sh
```

---

## 4. Paired Inter-MCU Log Relay Verification

The CORE-to-EARS diagnostic log relay is verified jointly with `PIUU_CORE` using actual compiled CORE logger output:

```bash
# Executed from PIUU_CORE directory
bash ../PIUU_CORE/test/run_ears_log_tests.sh
```

This verifies:
1. Identical header layout between `PIUU_CORE` and `C3_EARS` ([`src/core_log_wire.h`](../src/core_log_wire.h)).
2. Byte-by-byte fragmentation handling in [`test/log_relay/test_relay.cpp`](../test/log_relay/test_relay.cpp).
3. USB queue saturation protection and CRC rejection without interrupting audio control.

---

## 5. Documentation & Codebase Drift Auditing

To ensure zero drift across documentation manifests, markdown links, scripts, and agent recipes:

```bash
# Run drift auditor with detailed output
python3 tools/audit_skills_drift.py --verbose
```

- Verifies all markdown links resolve to existing files on disk.
- Validates all script and source file references.
- Checks agent recipe metadata headers and `--help` CLI compliance.
- Must return exit code `0` before any pull request or session handover.
