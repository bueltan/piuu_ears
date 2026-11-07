# Inter-MCU UART Protocol & Packet Routing Guide

> **Scope**: Serial wire protocols (`PCM!`, `EVT!`, `CMD!`), packet framing, state routing, concurrent audio distribution, and mutual exclusion across `C3_EARS`, `C3_KEYWORD`, and `PIUU_CORE`.

---

## 1. Dual UART Architecture & Line Specifications

`C3_EARS` operates two independent hardware UART interfaces at a fixed line rate of **921,600 baud (8N1)**:

1. **WAKE Link (UART1)**:
   - Dedicated connection to `C3_KEYWORD`.
   - TX: `GPIO4`, RX: `GPIO5`.
   - Transmits outgoing `PCM!` audio frames; receives incoming `EVT!` detection events.
2. **CORE Link (UART0)**:
   - Dedicated connection to `PIUU_CORE`.
   - TX: `GPIO6`, RX: `GPIO7`.
   - Transmits outgoing `PCM!` audio frames and relayed `EVT!` events; receives incoming `CMD!` commands and diagnostic log frames.

---

## 2. Binary Wire Framing Formats

All serial communication uses binary envelope headers with 4-byte ASCII magic markers.

### 2.1 Audio Framing: `PCM!` Packet

Carries 20 ms chunks of 16 kHz 16-bit mono PCM audio (defined in [`src/audio_protocol.h`](../src/audio_protocol.h)):

| Offset | Field | Type | Size | Description |
| :--- | :--- | :--- | :--- | :--- |
| `0..3` | `magic` | `uint8_t[4]` | 4 B | ASCII marker: `'P', 'C', 'M', '!'` (`0x50, 0x43, 0x4D, 0x21`). |
| `4..7` | `sequence` | `uint32_t` | 4 B | Monotonic global sequence number (little-endian). |
| `8..9` | `payloadBytes`| `uint16_t` | 2 B | Size of following PCM payload (typically `640` bytes). |
| `10..` | `pcmData` | `int16_t[]` | 640 B| Raw audio samples (320 samples, little-endian). |

- **Header Size**: Exactly 10 bytes (`sizeof(AudioFrameHeader) == 10`).
- **Total Packet Length**: 650 bytes per frame.

---

### 2.2 Event Framing: `EVT!` Packet

Carries wake-word detections, keyword triggers, and status updates (defined in [`src/control_protocol.h`](../src/control_protocol.h)):

| Offset | Field | Type | Size | Description |
| :--- | :--- | :--- | :--- | :--- |
| `0..3` | `magic` | `uint8_t[4]` | 4 B | ASCII marker: `'E', 'V', 'T', '!'` (`0x45, 0x56, 0x54, 0x21`). |
| `4` | `version` | `uint8_t` | 1 B | Protocol version: `EVENT_PROTOCOL_VERSION = 1`. |
| `5` | `event` | `uint8_t` | 1 B | Event identifier code (see table below). |
| `6..7` | `payloadBytes`| `uint16_t` | 2 B | Payload byte length (maximum 32 bytes). |
| `8..` | `payload` | `uint8_t[]` | Var | Optional event parameters or text string. |

- **Header Size**: Exactly 8 bytes (`sizeof(EventHeader) == 8`).

#### Defined Event Codes:
- `0x00`: `NONE`
- `0x01`: `WAKE_WORD` (Relayed to CORE; enables audio streaming).
- `0x02`: `LIGHTS_WORD`
- `0x03`: `STOP_WORD`
- `0x04`: `MUTE_WORD`
- `0x06`: `EYES_COMMAND` (Local developer command injected via USB console).
- `0x80`: `CORE_STREAM_STATE` (EARS &rarr; CORE acknowledgement; payload: `1`=Active, `0`=Idle).

---

### 2.3 Command Framing: `CMD!` Packet

Carries control instructions and diagnostics from `PIUU_CORE` to `C3_EARS` (defined in [`src/control_protocol.h`](../src/control_protocol.h)):

| Offset | Field | Type | Size | Description |
| :--- | :--- | :--- | :--- | :--- |
| `0..3` | `magic` | `uint8_t[4]` | 4 B | ASCII marker: `'C', 'M', 'D', '!'` (`0x43, 0x4D, 0x44, 0x21`). |
| `4` | `version` | `uint8_t` | 1 B | Protocol version: `COMMAND_PROTOCOL_VERSION = 1`. |
| `5` | `command` | `uint8_t` | 1 B | Command code (see table below). |
| `6..7` | `payloadBytes`| `uint16_t` | 2 B | Payload length (0 for routing commands; up to 211 bytes for log relay). |

- **Header Size**: Exactly 8 bytes (`sizeof(CommandHeader) == 8`).

#### Defined Command Codes:
- `0x01`: `START_AUDIO` — Direct streaming to CORE only (suppresses KEYWORD during speaker playback).
- `0x02`: `STOP_AUDIO` — Halts streaming to CORE; restores default streaming to KEYWORD.
- `0x03`: `START_AUDIO_WITH_KEYWORDS` — Concurrently streams PCM to both CORE and KEYWORD.
- `0x10`: `DIAGNOSTIC_LOG` — Envelopes CRC-protected logging output from CORE to EARS.

---

## 3. Audio Routing State Machine & Echo Guard

State transitions are governed by [`src/ears_state.cpp`](../src/ears_state.cpp):

```text
               +----------------------------------+
               |        IDLE / STANDBY            |
               | Routes: KEYWORD Only (Bit 1)     |
               +----------------------------------+
                     |                      ^
         EVT! 0x01   |                      |  CMD! 0x02 (STOP_AUDIO)
        (Wake Word)  |                      |
                     v                      |
  +--------------------------+  CMD! 0x01   +--------------------------+
  |    CONVERSATION ACTIVE   | -----------> |     SPEAKER ACTIVE       |
  | Routes: CORE + KEYWORD   |              | Routes: CORE Only (Bit 0)|
  | (CMD! 0x03 or Wake Word) | <----------- | (Suppresses Keyword Echo)|
  +--------------------------+  CMD! 0x03   +--------------------------+
```

1. **Boot Default**:
   - `streamToCore = false`, `streamToKeyword = true`.
   - `C3_KEYWORD` continuously monitors audio while CORE is idle.
2. **Wake Detection**:
   - When `C3_KEYWORD` matches the wake word, it emits `EVT! 0x01`.
   - `C3_EARS` forwards `EVT! 0x01` to CORE and sets `streamToCore = true`.
3. **Speaker Echo Guard**:
   - While `PIUU_CORE` plays TTS audio through its DAC, it issues `CMD! 0x01` (`START_AUDIO`).
   - This disables KEYWORD routing, preventing speaker acoustic feedback from triggering spurious wake events.
   - 250 ms after playback ends, CORE issues `CMD! 0x03` (`START_AUDIO_WITH_KEYWORDS`) to re-enable concurrent listening.

---

## 4. Concurrency & Mutex Serialization

`coreLinkSendAudio()` and `coreLinkForwardEvent()` both compete for `coreSerial` transmission.

- **`txMutex`**: FreeRTOS binary semaphore created in [`src/core_link.cpp`](../src/core_link.cpp).
- **Scope**: The calling thread locks `txMutex` across the **entire packet** (header + payload). This prevents a high-priority event or log line from fragmenting or interleaving inside a 650-byte PCM frame.
