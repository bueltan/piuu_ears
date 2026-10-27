#include "wake_link.h"
#include "audio_protocol.h"
#include "control_protocol.h"
#include "control_parser.h"
#include "core_link.h"
#include "config.h"

static HardwareSerial wakeSerial(1);

// Written and reported only by loop(), never by the audio task.
static uint32_t rxBytes = 0, parsedEvents = 0, invalidEvents = 0, relayErrors = 0;
static uint32_t lastReport = 0;
static uint8_t rxSample[16] = {};
static size_t rxSampleSize = 0;

static void reportRx(uint32_t now)
{
    if (uint32_t(now - lastReport) < 5000) return;
    lastReport = now;
    char hex[sizeof(rxSample) * 3 + 1] = {};
    static const char digits[] = "0123456789ABCDEF";
    for (size_t i = 0; i < rxSampleSize; ++i) {
        hex[i * 3] = digits[rxSample[i] >> 4];
        hex[i * 3 + 1] = digits[rxSample[i] & 15];
        hex[i * 3 + 2] = ' ';
    }
    Serial.printf("[WAKE_RX] bytes=%lu events=%lu invalid=%lu relay_errors=%lu sample=%s\n",
        (unsigned long)rxBytes, (unsigned long)parsedEvents,
        (unsigned long)invalidEvents, (unsigned long)relayErrors,
        rxSampleSize ? hex : "-");
    rxSampleSize = 0;
}

static void handleEvent(uint8_t version, uint8_t event, const uint8_t *payload, size_t size)
{
    ++parsedEvents;
    if (version != EVENT_PROTOCOL_VERSION || event == uint8_t(AudioEvent::NONE) ||
        event == uint8_t(AudioEvent::CORE_STREAM_STATE)) {
        ++invalidEvents;
        Serial.printf("[EARS] Invalid WAKE EVT version=%u event=%u\n", version, event);
        return;
    }
    // Relay known and future events without interpreting their opaque payload.
    const bool sent = coreLinkForwardEvent(event, payload, size);
    if (!sent) ++relayErrors;
    Serial.printf("[EARS] EVT=%u payload=%u relay=%s\n", event, unsigned(size), sent ? "OK" : "ERROR");
}

static ControlParser eventParser(EVT_MAGIC, handleEvent);

void wakeLinkBegin()
{
    wakeSerial.begin(UART_BAUD, SERIAL_8N1, WAKE_RX_PIN, WAKE_TX_PIN);
    Serial.printf("[EARS] WAKE UART ready RX=%d TX=%d @ %lu\n",
        WAKE_RX_PIN, WAKE_TX_PIN, static_cast<unsigned long>(UART_BAUD));
}

bool wakeLinkSendAudio(uint32_t sequence, const uint8_t *pcm, size_t size)
{
    return sendAudioFrame(wakeSerial, sequence, pcm, size);
}

void wakeLinkProcessRx()
{
    eventParser.expire(millis(), CONTROL_RX_TIMEOUT_MS);
    for (size_t n = 0; n < 256 && wakeSerial.available() > 0; ++n) {
        const int value = wakeSerial.read();
        if (value < 0) break;
        ++rxBytes;
        if (rxSampleSize < sizeof(rxSample)) rxSample[rxSampleSize++] = uint8_t(value);
        eventParser.feed(uint8_t(value), millis());
    }
    reportRx(millis());
}

// Only called with audio parked and loop RX dispatch disabled.
void wakeLinkPause() {
    wakeSerial.flush();wakeSerial.end();
    eventParser=ControlParser(EVT_MAGIC, handleEvent);
}
void wakeLinkResume() {
    wakeSerial.begin(UART_BAUD,SERIAL_8N1,WAKE_RX_PIN,WAKE_TX_PIN);
}
