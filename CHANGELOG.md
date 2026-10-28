# Changelog

## 2026-10-28

- Pause PCM routing to KEYWORD while streaming to CORE (`streamToCore=true`) to avoid wake-word inference during active conversations.
- Increase INMP441 microphone gain (`MIC_PCM_SHIFT=15`, doubling PCM amplitude) to improve pickup at distance.
- Add sample-paced microphone warmup mute (`EARS_MIC_STARTUP_MUTE_MS=1000`) to suppress initial power-on transients.

## 2026-10-27

- Integrate USB-triggered maintenance into normal EARS firmware with cooperative audio pause and UART handoff.
- Resume audio after acknowledged exit; keep failed uploads in maintenance without flash writes or automatic resets.

- Preserve CORE ROM boot output and add bridge diagnostics and transfer counters.
- Retain UART frames received alongside command acknowledgements.

- Add an isolated USB-to-CORE maintenance bridge with EN/BOOT control and bounded, acknowledged binary transport.
- Add a fixed-baud esptool upload helper, host tests and wiring/recovery instructions.

## 2026-10-22

- Add INMP441 I2S capture on GPIO0/1/3 as the default audio source.
- Preserve file replay selection and add optional microphone level/error summaries.

## 2026-10-11

- Add optional UART transmit timing and audio pacing summaries for CORE/KEYWORD routing.
