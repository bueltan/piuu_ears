# CORE Maintenance Bridge & In-System Flashing Guide

> **Scope**: Architecture of the integrated USB-to-UART flashing bridge, hardware strapping sequences, binary bridge protocol, and operations using `tools/core_bridge.py`.

---

## 1. Architectural Overview

`C3_EARS` eliminates the need to plug an external USB cable into `PIUU_CORE` by serving as an in-system hardware programmer and diagnostic tunnel.

```text
Host PC (USB) <=== [115200 baud] ===> ESP32-C3 (EARS) <=== [115200 baud UART] ===> ESP32 (CORE)
                                              |
                                              +---> CORE_EN_PIN   (GPIO10) -> Reset
                                              +---> CORE_BOOT_PIN (GPIO2)  -> GPIO0 (Boot)
```

The bridge exists in two forms:
1. **Integrated Bridge (`EARS_INTEGRATED_BRIDGE=1`)**: Active in standard `esp32-c3-ears` and `esp32-c3-ears-quiet`. Enters maintenance mode dynamically upon receiving the bridge invocation sequence, and resumes normal audio capture upon exit without rebooting EARS.
2. **Dedicated Bridge (`esp32-c3-core-bridge`)**: Standalone maintenance firmware environment built from [`src/bridge/main.cpp`](../src/bridge/main.cpp) for low-level board recovery.

---

## 2. Cooperative Audio Handoff & Safe Mode Transition

Before the bridge opens UART access to CORE:

1. **Detection**: Serial input in [`src/main.cpp`](../src/main.cpp) feeds `maintenanceRequest.feed()`.
2. **Cooperative Parking**:
   - `audioEnginePause()` requests the audio FreeRTOS task to park between 20 ms frame boundaries.
   - `earsStopCoreStream()` clears active routing flags.
   - `audioSourcePause()` disables I2S DMA.
   - `coreLinkPause()` and `wakeLinkPause()` flush and close UART peripherals.
3. **USB Mode Switch**:
   - Serial TX timeout is switched from `0 ms` (normal non-blocking log mode) to `100 ms` (acknowledged binary transport).
   - EARS outputs `PIUU BRIDGE READY\n` and initializes the bridge state machine.
4. **Resumption**:
   - Upon receiving the `Bridge::Exit` packet, EARS resets CORE into application mode, restores normal UART baud rates (921,600), resumes I2S DMA and audio tasks, and sets USB TX timeout back to `0 ms`.

---

## 3. Binary Bridge Protocol (`EB` Wire Format)

All bridge commands between `tools/core_bridge.py` and EARS are framed with an 7-byte envelope and CRC-CCITT (defined in [`src/bridge/protocol.h`](../src/bridge/protocol.h)):

| Offset | Field | Type | Size | Description |
| :--- | :--- | :--- | :--- | :--- |
| `0..1` | `header` | `uint8_t[2]` | 2 B | Magic header: `'E', 'B'` (`0x45, 0x42`). |
| `2` | `type` | `uint8_t` | 1 B | Packet type (see enumeration below). |
| `3..4` | `length` | `uint16_t` | 2 B | Payload byte length (little-endian; max 256 bytes). |
| `5..` | `data` | `uint8_t[]` | Var | Binary payload data. |
| `5+N..6+N` | `crc` | `uint16_t` | 2 B | CRC16-CCITT (poly `0x1021`, init `0xFFFF`) over `type + length + data`. |

### Packet Types:
- `1`: `Ping` — Host connection ping.
- `2`: `Boot` — Forces CORE into ROM bootloader via EN/BOOT lines.
- `3`: `Run` — Releases BOOT and resets CORE into normal application flash.
- `4`: `Data` — Binary data payload proxied between USB and CORE UART.
- `5`: `Ack` — Acknowledges received command or data block.
- `6`: `Error` — Reports CRC or framing error.
- `7`: `Exit` — Gracefully terminates bridge mode and resumes normal EARS operations.

---

## 4. Hardware Reset & Bootstrap Sequence

Resetting CORE into the ESP32 ROM bootloader requires precise timing:

1. `digitalWrite(CORE_EN_PIN, LOW)` (assert Reset).
2. `digitalWrite(CORE_BOOT_PIN, download ? LOW : HIGH)` (pull GPIO0 low for download).
3. Wait `100 ms` for power stabilization.
4. Initialize UART0 at `115,200 baud` (ROM bootloader default).
5. `digitalWrite(CORE_EN_PIN, HIGH)` (release Reset).
6. Wait `100 ms` for strap sampling to complete before releasing `CORE_BOOT_PIN` back to HIGH.

---

## 5. Tool Usage: `tools/core_bridge.py`

Flashing operations are driven via [`tools/core_bridge.py`](../tools/core_bridge.py):

```bash
# 1. Diagnose: Verify CORE ROM boot output without writing flash
python3 tools/core_bridge.py --port /dev/esp32-ears diagnose

# 2. Info: Read target chip ID and MAC
python3 tools/core_bridge.py --port /dev/esp32-ears info

# 3. Flash: Write full PlatformIO build artifacts from PIUU_CORE
python3 tools/core_bridge.py --port /dev/esp32-ears flash \
    --build-dir ../PIUU_CORE/.pio/build/cyd-uart0

# 4. Run: Explicitly reset CORE into application mode
python3 tools/core_bridge.py --port /dev/esp32-ears run
```

### Flash Offset Mapping
When flashing `PIUU_CORE`, the tool writes four distinct binaries:
- `0x1000`: `bootloader.bin`
- `0x8000`: `partitions.bin`
- `0xe000`: `boot_app0.bin`
- `0x10000`: `firmware.bin`
