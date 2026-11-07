# Changelog

## 2026-11-05 — Ganancia del micrófono

- Subir `MIC_GAIN_PERCENT` de 120 % a 200 % antes de la saturación PCM16.
- Documentar que la amplitud mayor puede incrementar el clipping en picos.

## 2026-11-02 — Ganancia configurable del micrófono

- Añadir `MIC_GAIN_PERCENT` en `config.h` para ajustar la ganancia digital
  del INMP441, con un valor inicial del 120 %.
- Aplicar la ganancia durante la conversión I2S a PCM16, utilizando
  aritmética entera y saturación para evitar desbordamientos.


## 2026-11-02 — Diagnóstico de arranque y reinicios de EARS

- Añadir un identificador único por arranque (`earsBootId`) y registrar
  la causa del reinicio mediante `esp_reset_reason()`.
- Incorporar mensajes periódicos `[EARS_ALIVE]` cada 5 segundos con
  identificador de arranque, uptime y motivo del último reinicio.


## 2026-11-01 — Variante quiet

- Añadir esp32-c3-ears-quiet con logs de aplicación, MIC/UART y relay CORE
  desactivados en compilación; mantener audio, eventos, ACK y mantenimiento.
- Centralizar salida diagnóstica en ears_log.h sin evaluar argumentos cuando
  está desactivada; silenciar logs SDK al iniciar la variante quiet.
- Probar routing, pausa/reanudación y serialización tanto normal como quiet.
- No modificar USB/DTR/RTS ni afirmar que se resuelva el reset al abrir el puerto.

## 2026-10-30

- Add local USB `EYES <command>` forwarding to PIUU_EYES via EVT! 0x06, separate from ordinary EARS expression events and maintenance. Commands require updated EARS and PIUU_EYES firmware; no automatic upload.

## 2026-10-29

- Decode CORE diagnostic CMD! 0x10 frames, validate CRC and forward prefixed chunks over native USB with an eight-fragment static queue and loss counters.
- Keep UART control/audio processing independent of USB backpressure; set normal USB TX timeout to zero and restore 100 ms for the acknowledged maintenance bridge.
- Expand CORE UART RX to 1024 bytes and command payload admission to 211 bytes; preserve KEYWORD's logical 32-byte limit and existing audio routing.
- Document CORE→EARS→USB capture and add interoperability/backpressure regressions using actual CORE logger output.

## 2026-10-28

- Route PCM concurrently to CORE and KEYWORD while listening or waiting for an AI response. CORE suppresses KEYWORD only during speaker playback and its 250 ms echo guard.
- Add `START_AUDIO_WITH_KEYWORDS=0x03`; keep legacy START as CORE-only and STOP as keyword-only. A repeated WAKE preserves the current routing instead of undoing speaker suppression.
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
