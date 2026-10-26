#pragma once

#include <Arduino.h>


enum class AudioSourceType
{
    FILE_REPLAY,
    MICROPHONE
};


static constexpr AudioSourceType AUDIO_SOURCE =
    AudioSourceType::MICROPHONE;


// ------------------------------------------------------------
// Audio
// ------------------------------------------------------------

static constexpr uint32_t SAMPLE_RATE_HZ = 16000;
static constexpr uint32_t FRAME_MS = 20;

static constexpr size_t FRAME_SAMPLES =
    (SAMPLE_RATE_HZ * FRAME_MS) / 1000;

static constexpr size_t FRAME_BYTES =
    FRAME_SAMPLES * sizeof(int16_t);

// INMP441 on ESP32-C3 SuperMini. L/R must be tied to 3V3 (right slot).
static constexpr int MIC_BCLK_PIN = 0;
static constexpr int MIC_WS_PIN = 1;
static constexpr int MIC_DATA_PIN = 3;
// Signed 24-bit microphone samples arrive left-aligned in 32-bit I2S slots.
// Shift 16 gives PCM16 at unity gain; 15 doubles gain, 14 quadruples it.
// Conversion saturates instead of wrapping when gain exceeds full scale.
static constexpr unsigned MIC_PCM_SHIFT = 16;
static_assert(MIC_PCM_SHIFT >= 8 && MIC_PCM_SHIFT <= 24, "Invalid microphone PCM shift");

#ifndef EARS_MIC_DIAGNOSTICS
#define EARS_MIC_DIAGNOSTICS 1
#endif


// ------------------------------------------------------------
// UART
// ------------------------------------------------------------

static constexpr uint32_t UART_BAUD = 921600;


// EARS <-> WAKE

static constexpr int WAKE_TX_PIN = 4;
static constexpr int WAKE_RX_PIN = 5;


// EARS <-> CORE

static constexpr int CORE_TX_PIN = 6;
static constexpr int CORE_RX_PIN = 7;

static_assert(MIC_BCLK_PIN != MIC_WS_PIN && MIC_BCLK_PIN != MIC_DATA_PIN &&
    MIC_WS_PIN != MIC_DATA_PIN, "Microphone pins must differ");
constexpr bool micPinConflictsWithUart(int pin)
{
    return pin == WAKE_TX_PIN || pin == WAKE_RX_PIN ||
        pin == CORE_TX_PIN || pin == CORE_RX_PIN;
}
static_assert(!micPinConflictsWithUart(MIC_BCLK_PIN) &&
    !micPinConflictsWithUart(MIC_WS_PIN) && !micPinConflictsWithUart(MIC_DATA_PIN),
    "Microphone and UART pins must differ");
