#include "ears_log.h"
#include <Arduino.h>
#include <esp_log.h>

#include "wake_link.h"
#include "core_link.h"
#include "audio_engine.h"
#include "uart_diagnostics.h"
#include "audio_source.h"
#include "ears_state.h"
#include "bridge/runtime.h"
#include "maintenance_request.h"
#include "eyes_usb_command.h"

#include <esp_system.h>

static bool maintenance = false;
static MaintenanceRequest maintenanceRequest;
static EyesUsbCommand eyesUsbCommand;

static uint32_t earsBootId = 0;
static esp_reset_reason_t earsResetReason;

void maintenanceResume()
{
    earsStopCoreStream();
    coreLinkResume();
    wakeLinkResume();
    if (audioSourceResume())
        audioEngineResume();
    else
        EARS_LOGLN("[EARS] Audio resume failed; maintenance remains available");
    maintenance = false;
    Serial.setTxTimeoutMs(0); // Normal diagnostics must never wait for USB.
    EARS_LOGLN("[EARS] Normal mode restored");
}
static bool pollMaintenance()
{
    if (maintenance)
    {
        bridgePoll();
        return true;
    }
    for (unsigned i = 0; i < 128 && Serial.available(); ++i)
    {
        const char byte = char(Serial.read());
        const auto eyesResult = eyesUsbCommand.feed(byte, millis());
        if (eyesResult == EyesUsbCommand::Command)
        {
            const size_t size = strlen(eyesUsbCommand.command);
            const bool sent = coreLinkForwardEvent(EyesUsbCommand::EventCode,
                                                   reinterpret_cast<const uint8_t *>(eyesUsbCommand.command), size);
            EARS_LOGF("[EYES_USB] %s: %s\n", sent ? "forwarded" : "TX failed", eyesUsbCommand.command);
        }
        else if (eyesResult == EyesUsbCommand::Invalid)
            EARS_LOGLN("[EYES_USB] use EYES followed by 1..31 printable command bytes");
        if (!maintenanceRequest.feed(byte, millis()))
            continue;
        if (!audioEnginePause())
        {
            EARS_LOGLN("[EARS] Maintenance rejected: audio did not stop");
            return false;
        }
        earsStopCoreStream();
        audioSourcePause();
        coreLinkPause();
        wakeLinkPause();
        Serial.setTxTimeoutMs(100); // Maintenance uses acknowledged transfers.
        Serial.flush();             // All normal log producers are now quiescent.
        bridgeBegin();
        maintenance = true;
        Serial.println("PIUU BRIDGE READY");
        Serial.flush();
        return true;
    }
    return false;
}

void setup()
{
#if !EARS_LOGS_ENABLED
    esp_log_level_set("*", ESP_LOG_NONE);
#endif
    Serial.setRxBufferSize(4096);
    Serial.setTxBufferSize(4096);
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0); // Includes competing microphone/UART diagnostics.

    delay(3000);
    EARS_LOGF(
        "[EARS_BOOT] boot=%08lx reset_reason=%d\n",
        (unsigned long)earsBootId,
        (int)earsResetReason);

    EARS_LOGLN();
    EARS_LOGLN(
        "=================================");

    EARS_LOGLN(
        " C3 EARS");

    EARS_LOGLN(
        "=================================");

    EARS_LOGF(
        "Device: %s | MAC esperada: %s\n",
        DEVICE_NAME,
        DEVICE_MAC);

    // ---------------------------------------------
    // Communication
    // ---------------------------------------------

    wakeLinkBegin();

    if (!coreLinkBegin())
    {
        EARS_LOGLN("[EARS] Cannot initialize CORE link");
        while (true)
            delay(1000);
    }

    // Give other controllers time to boot.
    delay(1000);

    // ---------------------------------------------
    // Audio
    // ---------------------------------------------

    if (!audioEngineBegin())
    {
        EARS_LOGLN(
            "[EARS] Fatal audio initialization error");

        while (true)
        {
            delay(1000);
        }
    }

    EARS_LOGLN(
        "[EARS] Ready");
}

void loop()
{
    if (pollMaintenance())
        return;

    /*
     * Process control information arriving
     * asynchronously from WAKE.
     */
    wakeLinkProcessRx();

    coreLinkProcessRx();

    UartDiagnostics::poll();

    // ---------------------------------------------
    // EARS heartbeat diagnostics
    // ---------------------------------------------

    static uint32_t lastHeartbeat = 0;

    const uint32_t now = millis();

    if (now - lastHeartbeat >= 5000)
    {
        lastHeartbeat = now;

        EARS_LOGF(
            "[EARS_ALIVE] boot=%08lx uptime=%lu reset_reason=%d\n",
            (unsigned long)earsBootId,
            (unsigned long)now,
            (int)earsResetReason);
    }

    delay(1);
}