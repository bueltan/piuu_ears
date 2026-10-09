#pragma once

#include <Arduino.h>


enum class AudioSourceType
{
    FILE_REPLAY,
    MICROPHONE
};


static constexpr AudioSourceType AUDIO_SOURCE =
    AudioSourceType::FILE_REPLAY;


// ------------------------------------------------------------
// Audio
// ------------------------------------------------------------

static constexpr uint32_t SAMPLE_RATE_HZ = 16000;
static constexpr uint32_t FRAME_MS = 20;

static constexpr size_t FRAME_SAMPLES =
    (SAMPLE_RATE_HZ * FRAME_MS) / 1000;

static constexpr size_t FRAME_BYTES =
    FRAME_SAMPLES * sizeof(int16_t);


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