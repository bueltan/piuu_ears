#pragma once

#include <Arduino.h>


// ============================================================
// Initialization
// ============================================================

void wakeLinkBegin();


// ============================================================
// RX: process events coming from WAKE
// ============================================================

void wakeLinkProcessRx();


// ============================================================
// TX: send PCM audio to WAKE
// ============================================================

bool wakeLinkSendAudio(
    uint32_t sequence,
    const uint8_t *pcm,
    size_t payloadBytes
);