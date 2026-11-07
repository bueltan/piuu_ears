# AGENTS.md — Protocol & Architecture Roadmap for C3 EARS

This document governs the operational rules, environment standards, and engineering workflows for any AI agent, language model, or pair-programming session intervening in the `C3_EARS/` repository.

Its primary objectives are:
- Preserving architectural integrity across continuous paired sessions.
- Enforcing context economy through surgical file reads and specialized skill modules.
- Ensuring deterministic compilation, flashing, and testing workflows for ESP32-C3 firmware.
- Preventing documentation and code drift through automated verification.

**Core Directive: Agents must never dump or ingest monolithic files (>20 KB) into active chat context. Always profile state via [`STATE.md`](STATE.md), navigate capabilities via [`SKILLS.md`](SKILLS.md), and follow domain guides in [`skills/`](skills/).**

---

## 1. Technical Environment & Baseline Constraints

- **Hardware Platform:** ESP32-C3 SuperMini (`esp32-c3-devkitm-1` board target, 160 MHz single-core RISC-V, 320 KB SRAM, 4 MB Flash).
- **Acoustic Sensor & Sampling:** INMP441 MEMS microphone on I2S0 (`BCLK=GPIO0`, `WS=GPIO1`, `DATA=GPIO3`, L/R tied to 3V3). 16,000 Hz, 16-bit signed Mono PCM, 20 ms frames (320 samples / 640 bytes).
- **Signal Processing Parameters:** Digital gain `MIC_GAIN_PERCENT = 200` (2.0x amplitude) with hard saturation; startup transient mute `EARS_MIC_STARTUP_MUTE_MS = 1000u`.
- **Inter-MCU Links:** Dual hardware UART interfaces at a fixed rate of **921,600 baud (8N1)**:
  - WAKE Link: UART1 (`TX=GPIO4`, `RX=GPIO5`) paired with `C3_KEYWORD`.
  - CORE Link: UART0 (`TX=GPIO6`, `RX=GPIO7`) paired with `PIUU_CORE`.
- **Hardware Flashing Straps:** Open-drain lines `CORE_EN_PIN = GPIO10` and `CORE_BOOT_PIN = GPIO2` for in-system flashing of `PIUU_CORE`.
- **Framework & Toolchain:** PlatformIO Core (`~/.platformio/penv/bin/pio`) on Arduino / ESP-IDF framework.
- **Host Environment:** Linux (x86_64), Python 3.12.3, GCC/G++ with C++11 standard for host tests.
- **Strict Technical English Standard:** All governance documents (`AGENTS.md`, `SKILLS.md`, `STATE.md`), modular guides in `skills/`, agent tools, recipes, and test scripts must be authored strictly in **technical English**. User chat exchanges may occur in the user's preferred language (e.g., Spanish).

---

## 2. Compilation, Build Targets & Flashing Hierarchy

### PlatformIO Target Environments

| Environment | Description | Logging Mode | Diagnostic Features | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| **`esp32-c3-ears`** *(Default)* | Standard Production Build | Enabled (`115200` USB CDC) | MIC summaries, UART timing, Heartbeat, Integrated Bridge | Primary production firmware with complete telemetry and flashing bridge. |
| **`esp32-c3-ears-quiet`** | Quiet Diagnostic Variant | Disabled (`EARS_LOGS_ENABLED=0`) | MIC/UART diagnostics compiled out, SDK logs silenced | Low-latency audio and benchmarking without serial logging overhead. |
| **`esp32-c3-core-bridge`** | Dedicated Maintenance Firmware | Dedicated binary transport | Exclusive USB-to-UART flash proxy for CORE | Emergency recovery and dedicated in-system programmer for CORE. |

### Build Commands
```bash
# Build default production firmware (esp32-c3-ears)
~/.platformio/penv/bin/pio run -e esp32-c3-ears

# Build quiet firmware without logging overhead
~/.platformio/penv/bin/pio run -e esp32-c3-ears-quiet

# Build dedicated maintenance bridge firmware
~/.platformio/penv/bin/pio run -e esp32-c3-core-bridge

# Clean build artifacts
~/.platformio/penv/bin/pio run -t clean
```

### Flashing Hierarchy
1. **Via Central Firmware Manager (Recommended Path):**
   - Use the central multi-MCU firmware manager:
     ```bash
     bash ../piuu-firmware-manager.sh
     # Option 2: Flash standard EARS
     # Option 9: Flash quiet EARS & CORE pair
     # Option 11: Flash CORE via EARS bridge, then EARS
     ```
2. **Via Direct USB Upload:**
   - Connect native USB to the ESP32-C3 port and upload directly:
     ```bash
     ~/.platformio/penv/bin/pio run -e esp32-c3-ears -t upload
     ```
3. **Flashing PIUU_CORE Through EARS:**
   - Execute the in-system flashing bridge tool:
     ```bash
     ~/.platformio/penv/bin/python tools/core_bridge.py --port /dev/esp32-ears flash --build-dir ../PIUU_CORE/.pio/build/cyd-uart0
     ```

---

## 3. Dynamic Skill Maps & Context Economy

Monolithic reads of files exceeding 20 KB (such as `README` or `CHANGELOG.md`) are strictly prohibited in day-to-day AI agent operations. Historical documentation is preserved on disk for human reference but bypassed by AI agents.

Agents must consult the master index at [`SKILLS.md`](SKILLS.md) and ingest targeted domain guides on demand:

- **Hardware & Pinouts:** Read [`skills/hardware_pinouts.md`](skills/hardware_pinouts.md) for ESP32-C3 SuperMini GPIO mapping, INMP441 I2S wiring, CORE reset/boot strapping lines, and USB CDC configuration.
- **Audio Pipeline:** Read [`skills/audio_pipeline.md`](skills/audio_pipeline.md) for 16 kHz PCM capture, 24-to-16 bit conversion, digital gain (200%), startup warmup mute, and file replay emulation.
- **Inter-MCU Protocol:** Read [`skills/uart_interconnect_protocol.md`](skills/uart_interconnect_protocol.md) for dual UART framing (`PCM!`, `EVT!`, `CMD!`), packet structures, routing states, and mutual exclusion.
- **CORE Flashing Bridge:** Read [`skills/core_maintenance_bridge.md`](skills/core_maintenance_bridge.md) for the integrated and dedicated flashing bridges, `EB` protocol framing, and `tools/core_bridge.py` usage.
- **Diagnostics & Relay:** Read [`skills/diagnostics_and_relay.md`](skills/diagnostics_and_relay.md) for the CORE-to-USB log relay (`CMD! 0x10`), system heartbeats, developer `EYES` USB commands, and quiet mode.
- **Testing & Verification:** Read [`skills/testing_and_verification.md`](skills/testing_and_verification.md) for host C++ unit test runners, mocks, regression coverage, and automated drift auditing.

---

## 4. State & Handover Protocol (`STATE.md`)

Operational continuity across sessions relies strictly on [`STATE.md`](STATE.md).

1. **Session Initialization Directive:** Upon beginning a task, read [`STATE.md`](STATE.md) to inspect the active firmware baseline, verified milestones, system configuration, and open issues.
2. **Session Closure Directive:** Before concluding, update [`STATE.md`](STATE.md) with:
   - Summary of completed modifications.
   - Test execution results.
   - Updated system configuration or open issues.
   - Recommended next steps for subsequent agents.
3. **Definition of Done (Drift Prevention):**
   - All unit and regression tests in `test/run_host_tests.sh` must pass with exit code `0`.
   - The drift auditor `python3 tools/audit_skills_drift.py --verbose` must report **0 errors**.
   - No dead links, unreferenced recipes, or missing script paths may exist.

---

## 5. Agent Tooling & Reusable Recipes (`tools/recipes/`)

To guarantee reproducible workflows and minimize hallucinated one-off scripts, agents must leverage and expand the recipe repository in [`tools/recipes/`](tools/recipes/):

1. **Standard Header Format:** Every script must declare standard metadata keys (`NAME:`, `PURPOSE:`, `USAGE:`, `IDEMPOTENT:`).
2. **Zero-Context Inspection:** Every recipe must support standard CLI `--help`.
3. **Guaranteed Idempotence:** Running a recipe multiple times with identical arguments must produce the same result without side effects.
4. **Starter Template:** Use [`tools/recipes/template_recipe.py`](tools/recipes/template_recipe.py) as the foundation for new agent automation tasks.
