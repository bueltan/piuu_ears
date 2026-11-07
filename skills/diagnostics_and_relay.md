# Diagnostics, Log Relay & Developer USB Commands

> **Scope**: Diagnostic subsystems in `C3_EARS`, including the CORE-to-USB log relay bridge, heartbeat telemetry, UART timing counters, developer USB `EYES` commands, and the quiet firmware profile.

---

## 1. CORE Log Relay Bridge (`CMD! 0x10`)

`PIUU_CORE` routes its runtime diagnostic output through `C3_EARS` over UART0/UART2 so that both microcontrollers can be monitored simultaneously through a single USB serial cable connected to `C3_EARS`.

### 1.1 Log Wire Format (`CoreLogWire`)
Defined in [`src/core_log_wire.h`](../src/core_log_wire.h):

| Offset | Field | Type | Size | Description |
| :--- | :--- | :--- | :--- | :--- |
| `0..3` | `bootId` | `uint32_t` | 4 B | CORE boot instance identifier (little-endian). |
| `4..7` | `sequence` | `uint32_t` | 4 B | Log chunk sequence number (monotonically increasing). |
| `8..11`| `uptimeMs` | `uint32_t` | 4 B | CORE uptime at log generation in milliseconds. |
| `12..15`| `dropped` | `uint32_t` | 4 B | Bytes dropped by CORE due to internal queue saturation. |
| `16` | `resetReason`| `uint8_t` | 1 B | ESP32 hardware reset reason code. |
| `17..N`| `text` | `char[]` | 1..192 B | ASCII log string fragment. |
| `N+1..N+2`| `crc` | `uint16_t` | 2 B | CRC16-CCITT (poly `0x1021`, init `0xFFFF`) over metadata + text. |

- **Header Envelope**: Carried inside standard 8-byte `CMD!` header (`command = 0x10`).
- **Total Payload**: 20 to 211 bytes.

### 1.2 Backpressure & Saturation Safety
Implemented in [`src/core_log_relay.cpp`](../src/core_log_relay.cpp):

1. **Dedicated Static Ring Buffer**:
   - Holds up to 8 fragments (maximum 192 bytes of text each).
   - If the queue fills up because the host computer is slow or disconnected, the oldest fragment is discarded and `usbDropped` is incremented.
2. **Zero USB Timeout (`setTxTimeoutMs(0)`)**:
   - Normal diagnostic printing never blocks. A disconnected or stalled USB host will **never** stall real-time I2S sampling or UART audio delivery.
3. **Control Byte Sanitization**:
   - Injected control characters (including newlines `\n` and `\r`) are sanitized to whitespace `' '`.
   - Guarantees exactly one USB line per received chunk, preventing CORE from spoofing EARS maintenance triggers (`PIUU BRIDGE READY`) or falsifying system status.

---

## 2. EARS System Telemetry & Heartbeat

When compiled with diagnostic logs enabled (`EARS_LOGS_ENABLED=1`):

1. **Heartbeat (`[EARS_ALIVE]`)**:
   - Emitted every 5,000 milliseconds from the main loop in [`src/main.cpp`](../src/main.cpp):
     ```text
     [EARS_ALIVE] boot=10b41d15 uptime=45000 reset_reason=1
     ```
   - Includes unique boot identifier (`earsBootId`), uptime, and ESP-IDF reset reason.
2. **WAKE Return Statistics (`[WAKE_RX]`)**:
   - Emitted every 5,000 ms in [`src/wake_link.cpp`](../src/wake_link.cpp):
     ```text
     [WAKE_RX] bytes=1280 events=4 invalid=0 relay_errors=0 sample=45 56 54 21 01 01 00 00
     ```
   - Reports total RX bytes from `C3_KEYWORD`, valid parsed events, invalid packets, and hex preview of the last frame.
3. **UART Timing Diagnostics (`[UART_TIMING]`)**:
   - Managed by [`src/uart_diagnostics.cpp`](../src/uart_diagnostics.cpp). Measures microsecond delivery latencies and jitter across both UART channels.

---

## 3. Local USB `EYES` Developer Commands

For testing eye animations, expressions, and gestures without issuing network requests or voice wake words:

- **Format**: Type `EYES <command>` directly into the EARS USB serial console (115,200 baud).
- **Processing**: [`src/eyes_usb_command.h`](../src/eyes_usb_command.h) buffers 1 to 31 printable characters.
- **Relay**: Automatically encapsulated into an `EVT! 0x06` packet and dispatched to CORE over UART0:
  ```text
  USB Serial in: "EYES happy\n"
  --> EARS parses command
  --> Encapsulates into EVT! 0x06 (payload="happy")
  --> Transmits over UART0 to PIUU_CORE
  ```

---

## 4. Quiet Firmware Profile (`esp32-c3-ears-quiet`)

For production deployments, benchmarks, and latency-critical environments:

- **Configuration**: Build target `esp32-c3-ears-quiet` in [`platformio.ini`](../platformio.ini).
- **Behavior**:
  - `EARS_LOGS_ENABLED = 0`: Application logs disabled via preprocessor macros in [`src/ears_log.h`](../src/ears_log.h).
  - `EARS_MIC_DIAGNOSTICS = 0`: Disables periodic microphone level summaries.
  - `EARS_UART_DIAGNOSTICS = 0`: Compiles out UART timing counters.
  - Silences ESP-IDF SDK log level at boot (`esp_log_level_set("*", ESP_LOG_NONE)`).
  - Preserves real-time audio capture, wake events, command processing, and maintenance bridge with zero serial overhead.
