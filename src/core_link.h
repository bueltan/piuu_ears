#pragma once

#include <Arduino.h>


void coreLinkBegin();


bool coreLinkSendAudio(
    uint32_t sequence,
    const uint8_t *pcm,
    size_t payloadBytes
);