#include "core_link.h"

#include "audio_protocol.h"
#include "config.h"


static HardwareSerial coreSerial(0);


// ============================================================
// Initialization
// ============================================================

void coreLinkBegin()
{
    coreSerial.begin(
        UART_BAUD,
        SERIAL_8N1,
        CORE_RX_PIN,
        CORE_TX_PIN
    );

    Serial.printf(
        "[EARS] CORE UART ready RX=%d TX=%d @ %lu\n",
        CORE_RX_PIN,
        CORE_TX_PIN,
        static_cast<unsigned long>(
            UART_BAUD
        )
    );
}


// ============================================================
// TX: PCM -> CORE
// ============================================================

bool coreLinkSendAudio(
    uint32_t sequence,
    const uint8_t *pcm,
    size_t payloadBytes
)
{
    return sendAudioFrame(
        coreSerial,
        sequence,
        pcm,
        payloadBytes
    );
}