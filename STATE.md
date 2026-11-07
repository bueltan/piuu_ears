# STATE.md — Project Progress & Firmware Checkpoint

> **Purpose**: This document tracks the active technical baseline, verified milestones, open issues, and immediate roadmap for `C3_EARS/`. It enables incoming AI agents and engineers to resume work seamlessly without context loss or redundant reverse-engineering.
>
> **Architecture Links**:
> - Operational Protocol & Standards: [`AGENTS.md`](AGENTS.md)
> - Master Capability Catalog: [`SKILLS.md`](SKILLS.md)

---

## 📍 Current System Baseline

- **Repository**: `C3_EARS` (Acoustic Capture, Audio Routing & Control Hub for PIUU Robot)
- **Root Directory**: `/home/denis/Documents/PlatformIO/Projects/C3_EARS/`
- **Active Firmware Releases**:
  - Production Build: `esp32-c3-ears` (default in [`platformio.ini`](platformio.ini); full diagnostics, integrated flashing bridge).
  - Quiet Diagnostic Variant: `esp32-c3-ears-quiet` (`-DEARS_LOGS_ENABLED=0`, `-DEARS_MIC_DIAGNOSTICS=0`, `-DEARS_UART_DIAGNOSTICS=0`).
  - Standalone Maintenance Bridge: `esp32-c3-core-bridge` (isolated USB-to-UART flash programmer).
- **Target Hardware**: ESP32-C3 SuperMini (Single-core RISC-V at 160 MHz, 320 KB SRAM, 4 MB Flash, LittleFS partition scheme in [`partitions_ears.csv`](partitions_ears.csv)).
- **Active Audio Specifications**:
  - Sample Rate: 16,000 Hz, 16-bit Mono signed PCM, little-endian.
  - Frame Duration: 20 ms (320 samples, 640 bytes per frame).
  - Acoustic Sensor: INMP441 I2S MEMS microphone (`BCLK=GPIO0`, `WS=GPIO1`, `DATA=GPIO3`, L/R tied to 3V3).
  - Digital Gain: `MIC_GAIN_PERCENT = 200` (2.0x gain before saturation to PCM16 `[-32768, 32767]`).
  - Warmup Transient Mute: `EARS_MIC_STARTUP_MUTE_MS = 1000u` (suppresses boot pop transients).
  - Deterministic Replay Fallback: `/test.raw` on LittleFS (`AudioSourceType::FILE_REPLAY`).
- **Inter-MCU UART Links (Fixed 921,600 baud, 8N1)**:
  - WAKE Link: UART1 (`TX=GPIO4`, `RX=GPIO5`) paired with `C3_KEYWORD`.
  - CORE Link: UART0 (`TX=GPIO6`, `RX=GPIO7`) paired with `PIUU_CORE` (`RX=GPIO3`, `TX=GPIO1` on CYD).
  - CORE Flashing Straps: `CORE_EN_PIN = GPIO10`, `CORE_BOOT_PIN = GPIO2` (open-drain lines).
  - Wire Protocols: `PCM!` (10-byte header, 640 bytes PCM), `EVT!` (8-byte header, up to 32 bytes), `CMD!` (8-byte header, up to 211 bytes).

---

## 📊 Test & Validation Status

Host unit and regression test suites executed on Linux (x86_64, GCC 13/C++11):

| Test Suite | Command | Result | Verified Capabilities |
| :--- | :--- | :--- | :--- |
| **Audio Routing & Links** | `bash test/run_audio_routing_tests.sh` | **PASS (exit 0)** | Cooperative pause, concurrent identical PCM, speaker echo suppression, wire framing, command timeouts, standard and quiet builds. |
| **Bridge Subsystem** | `bash test/run_bridge_tests.sh` | **PASS (exit 0)** | Reset sequence, strapping lines, binary fragment proxying, CRC16 validation, virtual socket tunnels, flow control, clean exit. |
| **Diagnostics & Hardware** | `bash test/run_diagnostics_tests.sh` | **PASS (exit 0)** | Developer USB `EYES` command parsing, UART timing diagnostics, 24-bit to 16-bit PCM conversion with gain saturation, 1000 ms warmup mute. |
| **Unified Host Suite** | `bash test/run_host_tests.sh` | **PASS (exit 0)** | End-to-end execution of all host test runners. |
| **Paired Log Relay** | `bash ../PIUU_CORE/test/run_ears_log_tests.sh` | **PASS (exit 0)** | CORE-to-EARS `CMD! 0x10` wire encoding, CRC verification, queue overflow protection, and offline USB saturation resilience. |
| **Drift Auditor** | `python3 tools/audit_skills_drift.py` | **PASS (exit 0)** | 100% Markdown link resolution, recipe docstrings, and script reference integrity. |

---

## ✅ Recently Verified Milestones

1. **Microphone Digital Gain Scaling (`MIC_GAIN_PERCENT = 200`)**:
   - Scaled digital gain to 200% (2.0x amplitude) during I2S 24-to-16 bit conversion in [`src/audio_source_microphone.cpp`](src/audio_source_microphone.cpp).
   - Validated saturation arithmetic in [`test/link_host/test_microphone_pcm.cpp`](test/link_host/test_microphone_pcm.cpp) to eliminate wraparound clipping.
2. **Boot ID & Reset Reason Telemetry (`[EARS_ALIVE]`)**:
   - Integrated unique boot identifier (`earsBootId`) and ESP-IDF reset reason logging every 5,000 ms in [`src/main.cpp`](src/main.cpp).
3. **Quiet Diagnostic Firmware Profile (`esp32-c3-ears-quiet`)**:
   - Implemented preprocessor macro gating in [`src/ears_log.h`](src/ears_log.h) and quiet target in [`platformio.ini`](platformio.ini).
   - Verified zero serial logging overhead while maintaining real-time audio routing, events, and maintenance mode.
4. **Local USB Developer `EYES` Command Tunnel**:
   - Added USB console command parser in [`src/eyes_usb_command.h`](src/eyes_usb_command.h) forwarding commands to CORE via `EVT! 0x06`.
5. **CORE-to-USB Diagnostic Log Relay (`CMD! 0x10`)**:
   - Created CRC16-protected logging pipeline in [`src/core_log_wire.h`](src/core_log_wire.h) and [`src/core_log_relay.cpp`](src/core_log_relay.cpp) with an 8-fragment static queue and non-blocking USB output.
6. **Concurrent Audio Routing & Speaker Echo Guard**:
   - Implemented `START_AUDIO_WITH_KEYWORDS` (`0x03`) to route PCM to both CORE and KEYWORD simultaneously during user turn-taking.
   - Preserved speaker-only routing during TTS playback to avoid acoustic feedback.
7. **Cooperative Integrated Maintenance Bridge**:
   - Integrated USB-to-UART flashing bridge into production firmware, allowing atomic parking of audio DMA tasks and in-system flashing of `PIUU_CORE`.

---

## ⚠️ Open Issues & Pending Validations

1. **Acoustic Gain & Ambient Noise Clipping**:
   - At 200% digital gain, high acoustic energy or close-range speech may saturate the PCM16 range (`[-32768, 32767]`). Physical testing in the target 3D-printed enclosure is required to confirm speech intelligibility.
2. **Host USB CDC Terminal Reset Behavior**:
   - Connecting or opening certain host serial terminals (e.g., Linux `picocom` or Python `pyserial`) without clearing DTR/RTS can trigger an unintended hardware reset of the ESP32-C3. Use `tools/core_bridge.py` or `piuu-monitor-helper.py` which explicitly clear DTR/RTS.
3. **Physical UART Long-Wire Signal Integrity**:
   - Inter-board wiring running at 921,600 baud requires short, direct traces/jumpers and clean ground bonding. Verify signal rise times on an oscilloscope if packet CRC errors are reported under vibration.

---

## ⏭️ Recommended Next Steps

1. **Physical Flashing & Serial Diagnostics**:
   - Deploy firmware using the central manager:
     ```bash
     bash ../piuu-firmware-manager.sh
     # Option 2: Flash standard EARS
     # Option 9: Flash quiet EARS & CORE pair
     # Option 11: Flash CORE via EARS bridge, then EARS
     ```
2. **Live Capture & Telemetry Monitoring**:
   - Monitor live system logs via:
     ```bash
     ~/.platformio/penv/bin/python ../PIUU_CORE/tools/capture_serial.py --only ears
     ```
3. **Automated Verification**:
   - Execute all host test runners before concluding any engineering session:
     ```bash
     bash test/run_host_tests.sh
     python3 tools/audit_skills_drift.py --verbose
     ```
