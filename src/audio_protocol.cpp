#include "audio_protocol.h"

#include <cstring>


bool sendAudioFrame(
    HardwareSerial &serial,
    uint32_t sequence,
    const uint8_t *pcm,
    size_t payloadBytes
)
{
    if (pcm == nullptr || payloadBytes == 0)
    {
        return false;
    }

    if (payloadBytes > UINT16_MAX)
    {
        return false;
    }

    AudioFrameHeader header;

    memcpy(
        header.magic,
        PCM_MAGIC,
        sizeof(PCM_MAGIC)
    );

    header.sequence = sequence;

    header.payloadBytes =
        static_cast<uint16_t>(payloadBytes);


    const size_t headerWritten =
        serial.write(
            reinterpret_cast<const uint8_t *>(&header),
            sizeof(header)
        );

    if (headerWritten != sizeof(header))
    {
        return false;
    }


    const size_t payloadWritten =
        serial.write(
            pcm,
            payloadBytes
        );

    if (payloadWritten != payloadBytes)
    {
        return false;
    }


    return true;
}