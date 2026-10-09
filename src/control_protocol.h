#pragma once

#include <Arduino.h>


static constexpr uint8_t EVT_MAGIC[4] = {
    'E', 'V', 'T', '!'
};

static constexpr uint8_t EVENT_PROTOCOL_VERSION = 1;

static constexpr size_t EVENT_MAX_PAYLOAD_BYTES = 32;


enum class AudioEvent : uint8_t
{
    NONE        = 0x00,
    WAKE_WORD   = 0x01,
    LIGHTS_WORD = 0x02,
    STOP_WORD   = 0x03,
    MUTE_WORD   = 0x04
};


struct __attribute__((packed)) EventHeader
{
    uint8_t magic[4];
    uint8_t version;
    uint8_t event;
    uint16_t payloadBytes;
};


static_assert(
    sizeof(EventHeader) == 8,
    "EventHeader debe ocupar 8 bytes"
);