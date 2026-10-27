#include "core_link.h"
#include "audio_protocol.h"
#include "control_parser.h"
#include "ears_state.h"
#include "config.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <cstring>

static HardwareSerial coreSerial(0);
static SemaphoreHandle_t txMutex = nullptr;

// Caller holds txMutex across the entire packet (header AND payload).
static bool sendEventLocked(uint8_t event, const uint8_t *payload, size_t size)
{
    if (size > EVENT_MAX_PAYLOAD_BYTES || (size && !payload)) return false;
    EventHeader header{};
    memcpy(header.magic, EVT_MAGIC, sizeof(EVT_MAGIC));
    header.version = EVENT_PROTOCOL_VERSION;
    header.event = event;
    header.payloadBytes = static_cast<uint16_t>(size);
    if (coreSerial.write(reinterpret_cast<const uint8_t *>(&header), sizeof(header)) != sizeof(header)) return false;
    return !size || coreSerial.write(payload, size) == size;
}

static void handleCommand(uint8_t version, uint8_t code, const uint8_t *, size_t size)
{
    // START/STOP v1 have no payload. Unknown/future commands must not change state.
    if (version != COMMAND_PROTOCOL_VERSION || size != 0 ||
        (code != uint8_t(CoreCommand::START_AUDIO) && code != uint8_t(CoreCommand::STOP_AUDIO))) {
        Serial.printf("[EARS] Unsupported CMD version=%u code=%u size=%u\n", version, code, unsigned(size));
        return;
    }
    if (!txMutex || xSemaphoreTake(txMutex, portMAX_DELAY) != pdTRUE) return;
    if (code == uint8_t(CoreCommand::START_AUDIO)) earsStartCoreStream();
    else earsStopCoreStream();
    const uint8_t active = earsIsStreamingToCore() ? 1 : 0;
    const bool sent = sendEventLocked(uint8_t(AudioEvent::CORE_STREAM_STATE), &active, 1);
    xSemaphoreGive(txMutex);
    Serial.printf("[EARS] CMD=%u core=%s state_tx=%d\n", code, active ? "ON" : "OFF", sent);
}

static ControlParser commandParser(CMD_MAGIC, handleCommand);

bool coreLinkBegin()
{
    if (txMutex) return true;
    txMutex = xSemaphoreCreateMutex();
    if (!txMutex) return false;
    coreSerial.setRxBufferSize(256);
    coreSerial.begin(UART_BAUD, SERIAL_8N1, CORE_RX_PIN, CORE_TX_PIN);
    Serial.printf("[EARS] CORE UART ready RX=%d TX=%d @ %lu\n",
        CORE_RX_PIN, CORE_TX_PIN, static_cast<unsigned long>(UART_BAUD));
    return true;
}

void coreLinkProcessRx()
{
    commandParser.expire(millis(), CONTROL_RX_TIMEOUT_MS);
    // Bound work so a noisy CORE cannot starve the WAKE link in loop().
    for (size_t n = 0; n < 256 && coreSerial.available() > 0; ++n) {
        const int value = coreSerial.read();
        if (value < 0) break;
        commandParser.feed(uint8_t(value), millis());
    }
}

bool coreLinkSendAudio(uint32_t sequence, const uint8_t *pcm, size_t size)
{
    if (!txMutex || xSemaphoreTake(txMutex, portMAX_DELAY) != pdTRUE) return false;
    // Check again under the lock: STOP may have arrived after audio_engine's check.
    const bool ok = !earsIsStreamingToCore() || sendAudioFrame(coreSerial, sequence, pcm, size);
    xSemaphoreGive(txMutex);
    return ok;
}

bool coreLinkForwardEvent(uint8_t event, const uint8_t *payload, size_t size)
{
    if (size > EVENT_MAX_PAYLOAD_BYTES || (size && !payload)) return false;
    if (!txMutex || xSemaphoreTake(txMutex, portMAX_DELAY) != pdTRUE) return false;
    const bool ok = sendEventLocked(event, payload, size);
    // Never enable initial PCM ahead of its wake event. On TX failure the caller
    // logs the failure; a subsequent wake or explicit START can recover.
    if (ok && event == uint8_t(AudioEvent::WAKE_WORD)) earsStartCoreStream();
    xSemaphoreGive(txMutex);
    return ok;
}

// Only called with audio parked and loop RX dispatch disabled.
void coreLinkPause() {
    coreSerial.flush();coreSerial.end();
    commandParser=ControlParser(CMD_MAGIC, handleCommand);
}
void coreLinkResume() {
    coreSerial.begin(UART_BAUD,SERIAL_8N1,CORE_RX_PIN,CORE_TX_PIN);
}
