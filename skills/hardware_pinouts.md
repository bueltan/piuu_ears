# Hardware & Pinout Architecture Guide

> **Scope**: Pinout definitions, electrical characteristics, strapping lines, and peripheral wiring for `C3_EARS` on the ESP32-C3 SuperMini platform.

---

## 1. ESP32-C3 SuperMini Pinout Matrix

The `C3_EARS` node runs on an ESP32-C3 SuperMini (single-core RISC-V at 160 MHz, 320 KB SRAM, 4 MB Flash). All pin assignments are centralized in [`src/config.h`](../src/config.h).

| Signal Group | Signal Name | ESP32-C3 GPIO | Electrical Type | Configuration & Notes |
| :--- | :--- | :--- | :--- | :--- |
| **I2S Microphone** | `MIC_BCLK_PIN` | **GPIO0** | Output (Clock) | Bit Clock for INMP441 MEMS microphone. |
| **I2S Microphone** | `MIC_WS_PIN` | **GPIO1** | Output (Word Select) | Frame clock (16 kHz, 20 ms frames). |
| **I2S Microphone** | `MIC_DATA_PIN` | **GPIO3** | Input (Data) | Serial PCM data stream from INMP441. |
| **WAKE UART** | `WAKE_TX_PIN` | **GPIO4** | Output (UART1 TX) | PCM stream out to `C3_KEYWORD` (921,600 baud, 8N1). |
| **WAKE UART** | `WAKE_RX_PIN` | **GPIO5** | Input (UART1 RX) | `EVT!` event stream from `C3_KEYWORD` (921,600 baud, 8N1). |
| **CORE UART** | `CORE_TX_PIN` | **GPIO6** | Output (UART0 TX) | Audio/Events out to `PIUU_CORE` (921,600 baud, 8N1). |
| **CORE UART** | `CORE_RX_PIN` | **GPIO7** | Input (UART0 RX) | `CMD!` commands & log frames from `PIUU_CORE` (921,600 baud, 8N1). |
| **CORE Strap Control** | `CORE_EN_PIN` | **GPIO10** | Open-Drain Output | Target MCU hardware Reset line (`EN`). Active low. |
| **CORE Strap Control** | `CORE_BOOT_PIN`| **GPIO2** | Open-Drain Output | Target MCU bootstrap mode line (`GPIO0` on ESP32-WROOM-32). Active low. |
| **USB CDC Console** | Native USB D+/D- | **GPIO18/GPIO19**| USB CDC On-Boot | Hardware USB serial for logs, bridge maintenance, and `EYES` debug. |

---

## 2. INMP441 MEMS Microphone Wiring

The INMP441 is an omnidirectional I2S digital microphone with a 24-bit output:

- **VCC**: Connect to `3V3` (3.3V rail).
- **GND**: Connect to common system ground.
- **SD (Serial Data)**: Connect to `GPIO3`.
- **SCK (Serial Clock)**: Connect to `GPIO0`.
- **WS (Word Select)**: Connect to `GPIO1`.
- **L/R (Channel Select)**: **Must be tied to `3V3`**. This selects the **right slot** in the I2S frame, aligning with the ESP32-C3 I2S driver configuration.

> [!IMPORTANT]
> The microphone and UART pins must never conflict. The compile-time assertions in [`src/config.h`](../src/config.h) enforce strict separation between microphone lines and inter-MCU communication buses.

---

## 3. CORE Flashing & Strap Control Lines

`C3_EARS` functions as an in-system flashing adapter for `PIUU_CORE` over UART0/UART2:

- **`CORE_EN_PIN` (GPIO10)**: Wired to the ESP32 `EN` (Reset) line of `PIUU_CORE`.
- **`CORE_BOOT_PIN` (GPIO2)**: Wired to `GPIO0` (Bootstrap pin) of `PIUU_CORE`.
- Both pins are driven as **open-drain outputs** (`OUTPUT_OPEN_DRAIN`):
  - In normal operation, both pins are driven HIGH (high-impedance released) allowing external on-board pull-up resistors to keep CORE in normal execution mode.
  - During maintenance upload, `CORE_BOOT_PIN` is pulled LOW before `CORE_EN_PIN` is pulsed LOW for 100 ms to latch CORE ROM bootloader mode.

---

## 4. Inter-MCU Physical Wiring Contract

All inter-chip serial connections require a common reference ground and direct 3.3V CMOS logic (no level-shifters needed):

```text
  +------------------+                    +------------------+
  |    C3_EARS       |                    |    PIUU_CORE     |
  |  (ESP32-C3)      |                    | (ESP32-WROOM-32) |
  |                  |                    |                  |
  |  CORE_TX (GPIO6) |------------------->| UART0_RX (GPIO3) |
  |  CORE_RX (GPIO7) |<-------------------| UART0_TX (GPIO1) |
  |  CORE_EN (GPIO10)|------------------->| EN (Reset)       |
  | CORE_BOOT (GPIO2)|------------------->| GPIO0 (Boot)     |
  |  GND             |--------------------| GND              |
  +------------------+                    +------------------+

  +------------------+                    +------------------+
  |    C3_EARS       |                    |    C3_KEYWORD    |
  |  (ESP32-C3)      |                    |    (ESP32-C3)    |
  |                  |                    |                  |
  |  WAKE_TX (GPIO4) |------------------->| RX (GPIO5)       |
  |  WAKE_RX (GPIO5) |<-------------------| TX (GPIO4)       |
  |  GND             |--------------------| GND              |
  +------------------+                    +------------------+
```

*(Note: On CYD configurations using UART2, CORE connects to `RX=GPIO16` and `TX=GPIO17`).*
