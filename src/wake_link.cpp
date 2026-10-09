#include "wake_link.h"

#include "audio_protocol.h"
#include "control_protocol.h"
#include "ears_state.h"
#include "config.h"

#include <cstring>


static HardwareSerial wakeSerial(1);


// ============================================================
// RX parser state
// ============================================================

enum class EventRxState : uint8_t
{
    SEARCH_MAGIC,
    READ_HEADER,
    READ_PAYLOAD,
    DISCARD_PAYLOAD
};


static EventRxState rxState =
    EventRxState::SEARCH_MAGIC;


static size_t magicMatched = 0;

static EventHeader eventHeader;

static size_t eventHeaderReceived = 0;


static uint8_t eventPayload[
    EVENT_MAX_PAYLOAD_BYTES
];

static size_t eventPayloadReceived = 0;

static size_t eventPayloadRemaining = 0;


// ============================================================
// Event handling
// ============================================================

static void handleEvent(
    const EventHeader &header,
    const uint8_t *payload,
    size_t payloadBytes
)
{
    if (header.version != EVENT_PROTOCOL_VERSION)
    {
        Serial.printf(
            "[EARS] Unsupported EVT version: %u\n",
            header.version
        );

        return;
    }


    const AudioEvent event =
        static_cast<AudioEvent>(
            header.event
        );


    switch (event)
    {
        case AudioEvent::WAKE_WORD:

            Serial.printf(
                "[EARS] WAKE_WORD received payload=%u\n",
                static_cast<unsigned>(
                    payloadBytes
                )
            );

            earsStartCoreStream();

            break;


        case AudioEvent::LIGHTS_WORD:

            Serial.printf(
                "[EARS] LIGHTS_WORD received payload=%u\n",
                static_cast<unsigned>(
                    payloadBytes
                )
            );

            /*
             * Futuro:
             * reenviar este evento hacia CORE.
             */

            break;


        case AudioEvent::STOP_WORD:

            Serial.printf(
                "[EARS] STOP_WORD received payload=%u\n",
                static_cast<unsigned>(
                    payloadBytes
                )
            );

            break;


        case AudioEvent::MUTE_WORD:

            Serial.printf(
                "[EARS] MUTE_WORD received payload=%u\n",
                static_cast<unsigned>(
                    payloadBytes
                )
            );

            break;


        default:

            Serial.printf(
                "[EARS] Unknown audio event: %u\n",
                header.event
            );

            break;
    }


    /*
     * Por ahora el payload se recibe correctamente
     * pero todavía no lo interpretamos.
     *
     * Más adelante podríamos usarlo para confidence:
     *
     * if (payloadBytes == sizeof(float))
     * {
     *     float confidence;
     *
     *     memcpy(
     *         &confidence,
     *         payload,
     *         sizeof(confidence)
     *     );
     * }
     */

    (void)payload;
}


// ============================================================
// Initialization
// ============================================================

void wakeLinkBegin()
{
    wakeSerial.begin(
        UART_BAUD,
        SERIAL_8N1,
        WAKE_RX_PIN,
        WAKE_TX_PIN
    );


    Serial.printf(
        "[EARS] WAKE UART ready RX=%d TX=%d @ %lu\n",
        WAKE_RX_PIN,
        WAKE_TX_PIN,
        static_cast<unsigned long>(
            UART_BAUD
        )
    );
}


// ============================================================
// TX: PCM -> WAKE
// ============================================================

bool wakeLinkSendAudio(
    uint32_t sequence,
    const uint8_t *pcm,
    size_t payloadBytes
)
{
    return sendAudioFrame(
        wakeSerial,
        sequence,
        pcm,
        payloadBytes
    );
}


// ============================================================
// RX: EVT! <- WAKE
// ============================================================

void wakeLinkProcessRx()
{
    while (wakeSerial.available() > 0)
    {
        const int value =
            wakeSerial.read();


        if (value < 0)
        {
            break;
        }


        const uint8_t byte =
            static_cast<uint8_t>(
                value
            );


        // ====================================================
        // 1. SEARCH_MAGIC
        // ====================================================

        if (
            rxState ==
            EventRxState::SEARCH_MAGIC
        )
        {
            if (
                byte ==
                EVT_MAGIC[magicMatched]
            )
            {
                magicMatched++;


                if (
                    magicMatched ==
                    sizeof(EVT_MAGIC)
                )
                {
                    memcpy(
                        eventHeader.magic,
                        EVT_MAGIC,
                        sizeof(EVT_MAGIC)
                    );


                    eventHeaderReceived =
                        sizeof(EVT_MAGIC);


                    magicMatched = 0;


                    rxState =
                        EventRxState::READ_HEADER;
                }
            }
            else
            {
                /*
                 * Si el byte actual también es 'E',
                 * puede ser el comienzo de un nuevo "EVT!".
                 */

                magicMatched =
                    (
                        byte ==
                        EVT_MAGIC[0]
                    )
                    ? 1
                    : 0;
            }


            continue;
        }


        // ====================================================
        // 2. READ_HEADER
        // ====================================================

        if (
            rxState ==
            EventRxState::READ_HEADER
        )
        {
            reinterpret_cast<uint8_t *>(
                &eventHeader
            )[eventHeaderReceived++] =
                byte;


            if (
                eventHeaderReceived <
                sizeof(EventHeader)
            )
            {
                continue;
            }


            /*
             * Header completo.
             */
            eventHeaderReceived = 0;


            // ------------------------------------------------
            // Event without payload
            // ------------------------------------------------

            if (
                eventHeader.payloadBytes == 0
            )
            {
                handleEvent(
                    eventHeader,
                    nullptr,
                    0
                );


                rxState =
                    EventRxState::SEARCH_MAGIC;


                continue;
            }


            // ------------------------------------------------
            // Payload fits in our buffer
            // ------------------------------------------------

            if (
                eventHeader.payloadBytes <=
                EVENT_MAX_PAYLOAD_BYTES
            )
            {
                eventPayloadReceived = 0;


                rxState =
                    EventRxState::READ_PAYLOAD;


                continue;
            }


            // ------------------------------------------------
            // Payload too large
            // ------------------------------------------------

            Serial.printf(
                "[EARS] EVT payload too large: "
                "%u bytes, discarding\n",
                eventHeader.payloadBytes
            );


            eventPayloadRemaining =
                eventHeader.payloadBytes;


            rxState =
                EventRxState::DISCARD_PAYLOAD;


            continue;
        }


        // ====================================================
        // 3. READ_PAYLOAD
        // ====================================================

        if (
            rxState ==
            EventRxState::READ_PAYLOAD
        )
        {
            eventPayload[
                eventPayloadReceived++
            ] = byte;


            if (
                eventPayloadReceived <
                eventHeader.payloadBytes
            )
            {
                continue;
            }


            /*
             * Payload completo.
             */
            handleEvent(
                eventHeader,
                eventPayload,
                eventPayloadReceived
            );


            eventPayloadReceived = 0;


            rxState =
                EventRxState::SEARCH_MAGIC;


            continue;
        }


        // ====================================================
        // 4. DISCARD_PAYLOAD
        // ====================================================

        if (
            rxState ==
            EventRxState::DISCARD_PAYLOAD
        )
        {
            if (
                eventPayloadRemaining > 0
            )
            {
                eventPayloadRemaining--;
            }


            if (
                eventPayloadRemaining == 0
            )
            {
                rxState =
                    EventRxState::SEARCH_MAGIC;


                magicMatched = 0;
            }


            continue;
        }
    }
}