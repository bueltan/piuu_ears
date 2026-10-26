#pragma once

#include <stdint.h>

// INMP441: signed 24-bit data in the most significant bits of a 32-bit slot.
inline int16_t microphonePcm16(int32_t raw, unsigned shift)
{
    const int32_t scaled = raw >> shift;
    if (scaled > INT16_MAX) return INT16_MAX;
    if (scaled < INT16_MIN) return INT16_MIN;
    return static_cast<int16_t>(scaled);
}
