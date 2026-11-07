# Audio Pipeline & Signal Processing Guide

> **Scope**: Acoustic sampling parameters, I2S capture pipeline, sample conversion, gain tuning, warmup transient muting, and file replay emulation in `C3_EARS`.

---

## 1. Acoustic Specifications & Framing Standards

`C3_EARS` acts as the primary acoustic input front-end for the PIUU robotics architecture:

- **Sample Rate**: 16,000 Hz (16 kHz).
- **Bit Depth & Format**: 16-bit signed PCM, little-endian, single channel (Mono).
- **Frame Duration**: 20 milliseconds (`FRAME_MS = 20`).
- **Samples per Frame**: 320 samples (`FRAME_SAMPLES = (16000 * 20) / 1000`).
- **Bytes per Frame**: 640 bytes (`FRAME_BYTES = 320 * sizeof(int16_t)`).
- **Global Frame Sequence**: Monotonically increasing `uint32_t` sequence number attached to each frame header. The sequence never resets during stream pause/resume or file replay looping.

---

## 2. INMP441 I2S Capture & Bit-Depth Conversion

The INMP441 MEMS microphone delivers 24-bit two's complement samples left-aligned within 32-bit I2S frame slots:

```text
  32-bit Slot: [ B23 B22 B21 ... B0 | 0 0 0 0 0 0 0 0 ]
```

Implementation resides in [`src/audio_source_microphone.cpp`](../src/audio_source_microphone.cpp) and [`src/microphone_pcm.h`](../src/microphone_pcm.h):

1. **Unity Shift**:
   - `MIC_PCM_SHIFT = 16` extracts the top 16 bits of the 32-bit slot (`raw >> 16`), providing unity gain.
2. **Gain Scaling**:
   - `MIC_GAIN_PERCENT = 200` applies a 2.0x digital gain multiplier before saturation.
   - Calculated using 32-bit integer arithmetic:
     $$\text{amplified} = \frac{\text{sample} \times \text{gain}}{100}$$
3. **Hard Saturation (Clipping Protection)**:
   - To prevent audible wraparound distortion (where positive peaks wrap to negative values), the conversion function saturates hard to `INT16_MAX` (`32767`) and `INT16_MIN` (`-32768`).

---

## 3. Power-On Warmup Mute

MEMS microphones experience electrical settling and capacitor charge transients upon startup, generating large DC offsets or acoustic pops:

- **Setting**: `EARS_MIC_STARTUP_MUTE_MS = 1000u` (in [`src/config.h`](../src/config.h)).
- **Mechanism**: Implemented in [`src/microphone_startup.h`](../src/microphone_startup.h). For the first 1,000 milliseconds (50 frames at 20 ms/frame), microphone samples are zeroed out (`0x00`) while preserving frame pacing and sequence timing.
- **Benefit**: Prevents spurious wake-word false positives during device boot.

---

## 4. Alternative Audio Source: File Replay

When hardware testing requires deterministic acoustic input without background noise, the system can replay raw PCM files from the on-board LittleFS partition:

- **File Path**: `/test.raw` (16 kHz, 16-bit mono little-endian raw PCM).
- **Configuration**: Change `AUDIO_SOURCE` in [`src/config.h`](../src/config.h) to `AudioSourceType::FILE_REPLAY`.
- **Looping Behavior**: When EOF is reached, the file rewinds and reopens seamlessly. The global `sequence` counter continues incrementing without discontinuity.

---

## 5. Audio Engine Task & Cooperative Pause

The core audio task runs on FreeRTOS:

- **Task Loop**: [`src/audio_engine.cpp`](../src/audio_engine.cpp) reads one 640-byte frame from the active audio source (`audioSourceReadFrame`).
- **Route Dispatch**:
  - Checks active routes via `earsAudioRoutes()`:
    - Bit 0 (`EARS_ROUTE_CORE = 1`): Forwards frame to `PIUU_CORE` via `coreLinkSendAudio()`.
    - Bit 1 (`EARS_ROUTE_KEYWORD = 2`): Forwards frame to `C3_KEYWORD` via `wakeLinkSendAudio()`.
- **Atomic Cooperative Parking**:
  - Maintenance flashing or calibration requires exclusive UART access.
  - Requesting `audioEnginePause()` signals atomic flags (`pauseRequested`, `parked`), safely parking the audio task between frame boundaries without killing the FreeRTOS task or corrupting DMA buffers.
