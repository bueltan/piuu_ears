#pragma once

#include <Arduino.h>


static constexpr uint8_t PCM_MAGIC[4] = {
    'P', 'C', 'M', '!'
};


struct __attribute__((packed)) AudioFrameHeader
{
    uint8_t magic[4];
    uint32_t sequence;
    uint16_t payloadBytes;
};


static_assert(
    sizeof(AudioFrameHeader) == 10,
    "AudioFrameHeader debe ocupar 10 bytes"
);


bool sendAudioFrame(
    HardwareSerial &serial,
    uint32_t sequence,
    const uint8_t *pcm,
    size_t payloadBytes
);