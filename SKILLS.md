# SKILLS.md — Master Capability Index & Skill Catalog

> **Governance Notice**: This document serves as the master catalog and architectural index for all domain skills, technical guides, and automation protocols within `C3_EARS/`. It links directly with [`AGENTS.md`](AGENTS.md) (operational protocol) and [`STATE.md`](STATE.md) (runtime checkpoint).

---

## 🧭 Navigation & Context Ingestion Architecture

Do **not** dump entire technical rulebooks into the active chat context. Agents must read [`AGENTS.md`](AGENTS.md) and [`STATE.md`](STATE.md) first upon initialization, and then ingest specific skill files from [`skills/`](skills/) on demand based on the active task domain.

```text
                      ┌───────────────┐
                      │   AGENTS.md   │ ◄── Core operational protocol & tools
                      └───────┬───────┘
                              │
             ┌────────────────┴────────────────┐
             ▼                                 ▼
    ┌─────────────────┐               ┌─────────────────┐
    │    SKILLS.md    │ ◄─ You are here│    STATE.md     │ ◄── Active firmware checkpoint
    └────────┬────────┘               └─────────────────┘
             │
             ├──► skills/hardware_pinouts.md          (ESP32-C3 SuperMini GPIOs, INMP441, Straps)
             ├──► skills/audio_pipeline.md            (16kHz PCM, I2S capture, 200% gain, warmup)
             ├──► skills/uart_interconnect_protocol.md(921600 baud, PCM!, EVT!, CMD!, routing)
             ├──► skills/core_maintenance_bridge.md   (Integrated flasher, EB wire format, core_bridge.py)
             ├──► skills/diagnostics_and_relay.md     (CORE log relay 0x10, heartbeats, USB EYES)
             └──► skills/testing_and_verification.md  (Host C++ unit runners, mocks, drift auditor)
```

---

## 📚 Skill Catalog Matrix

| Skill Module | Focus Area | When to Ingest | Reference Guide |
| :--- | :--- | :--- | :--- |
| **Hardware & Pinouts** | ESP32-C3 SuperMini pinout matrix, INMP441 I2S wiring, CORE reset/boot strapping lines, USB CDC. | Modifying GPIO assignments, diagnosing electrical conflicts, wiring new microphone hardware. | [`skills/hardware_pinouts.md`](skills/hardware_pinouts.md) |
| **Audio Pipeline & Capture** | 16 kHz Mono PCM16, INMP441 24-to-16 bit conversion, digital gain (200%), warmup transient mute, LittleFS file replay. | Tuning microphone sensitivity, debugging clipping/noise, adjusting audio frame buffers. | [`skills/audio_pipeline.md`](skills/audio_pipeline.md) |
| **Inter-MCU UART Protocol** | Dual UART architecture (921,600 baud, 8N1), binary envelopes (`PCM!`, `EVT!`, `CMD!`), packet formats, audio routing state machine. | Modifying inter-chip communication, debugging dropped wake events, investigating speaker echo guard. | [`skills/uart_interconnect_protocol.md`](skills/uart_interconnect_protocol.md) |
| **CORE Maintenance Bridge** | Integrated USB-to-UART flash bridge, cooperative audio parking, strapping sequences, `EB` protocol, `tools/core_bridge.py`. | Flashing `PIUU_CORE` through EARS, updating flashing routines, recovering unbootable CYD boards. | [`skills/core_maintenance_bridge.md`](skills/core_maintenance_bridge.md) |
| **Diagnostics & Log Relay** | CORE-to-USB log relay (`CMD! 0x10`), 8-fragment static queue, telemetry heartbeats (`[EARS_ALIVE]`), developer USB `EYES` commands, quiet build. | Monitoring real-time device logs, adding diagnostic counters, using USB developer commands. | [`skills/diagnostics_and_relay.md`](skills/diagnostics_and_relay.md) |
| **Testing & Verification** | Host C++ test runners (`run_host_tests.sh`), mocks, regression test suites, paired log relay testing, drift auditor. | Running automated tests, adding new regression unit tests, validating documentation drift. | [`skills/testing_and_verification.md`](skills/testing_and_verification.md) |

---

## 🛠️ Quick Task-to-Skill Dispatch

- **"Where are the I2S microphone and UART pins configured?"** &rarr; Read [`skills/hardware_pinouts.md`](skills/hardware_pinouts.md) &sect; 1.
- **"How does EARS control CORE's reset and bootloader modes?"** &rarr; Read [`skills/hardware_pinouts.md`](skills/hardware_pinouts.md) &sect; 3 and [`skills/core_maintenance_bridge.md`](skills/core_maintenance_bridge.md) &sect; 4.
- **"How is the 24-bit microphone signal converted to 16-bit PCM with 200% gain?"** &rarr; Read [`skills/audio_pipeline.md`](skills/audio_pipeline.md) &sect; 2.
- **"How do I simulate audio input using a file instead of the microphone?"** &rarr; Read [`skills/audio_pipeline.md`](skills/audio_pipeline.md) &sect; 4.
- **"What are the header sizes and byte layouts for PCM!, EVT!, and CMD! packets?"** &rarr; Read [`skills/uart_interconnect_protocol.md`](skills/uart_interconnect_protocol.md) &sect; 2.
- **"How does speaker playback suppress wake word detection to avoid acoustic feedback?"** &rarr; Read [`skills/uart_interconnect_protocol.md`](skills/uart_interconnect_protocol.md) &sect; 3.
- **"How do I flash PIUU_CORE using the Python bridge tool?"** &rarr; Read [`skills/core_maintenance_bridge.md`](skills/core_maintenance_bridge.md) &sect; 5.
- **"How are PIUU_CORE logs relayed to the host PC via EARS USB?"** &rarr; Read [`skills/diagnostics_and_relay.md`](skills/diagnostics_and_relay.md) &sect; 1.
- **"How do I send developer eye animation commands via USB?"** &rarr; Read [`skills/diagnostics_and_relay.md`](skills/diagnostics_and_relay.md) &sect; 3.
- **"How do I build the quiet firmware without logging overhead?"** &rarr; Read [`AGENTS.md`](AGENTS.md) &sect; 2 and [`skills/diagnostics_and_relay.md`](skills/diagnostics_and_relay.md) &sect; 4.
- **"How do I run all host unit and regression tests on Linux?"** &rarr; Read [`skills/testing_and_verification.md`](skills/testing_and_verification.md) &sect; 3.
- **"How do I verify documentation and recipe integrity?"** &rarr; Run `python3 tools/audit_skills_drift.py --verbose`.

---

## 🔄 Lifecycle & Handover Integration

After completing any task using these skills:
1. Verify postconditions using the verification routines specified in each skill guide.
2. Execute the host test suite (`bash test/run_host_tests.sh`) to prevent regressions.
3. Run `python3 tools/audit_skills_drift.py --verbose` to guarantee 0 documentation or recipe drift.
4. Update [`STATE.md`](STATE.md) with active milestones, configuration changes, and recommended next steps before closing the session.
