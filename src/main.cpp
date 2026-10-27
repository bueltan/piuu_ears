#include <Arduino.h>

#include "wake_link.h"
#include "core_link.h"
#include "audio_engine.h"
#include "uart_diagnostics.h"
#include "audio_source.h"
#include "ears_state.h"
#include "bridge/runtime.h"
#include "maintenance_request.h"
static bool maintenance=false;
static MaintenanceRequest maintenanceRequest;

void maintenanceResume() {
    earsStopCoreStream();
    coreLinkResume();wakeLinkResume();
    if(audioSourceResume())audioEngineResume();
    else Serial.println("[EARS] Audio resume failed; maintenance remains available");
    maintenance=false;
    Serial.println("[EARS] Normal mode restored");
}
static bool pollMaintenance() {
    if(maintenance){bridgePoll();return true;}
    for(unsigned i=0;i<128 && Serial.available();++i) {
        if(!maintenanceRequest.feed(char(Serial.read()),millis()))continue;
        if(!audioEnginePause()) {
            Serial.println("[EARS] Maintenance rejected: audio did not stop");return false;
        }
        earsStopCoreStream();
        audioSourcePause();
        coreLinkPause();wakeLinkPause();
        Serial.flush(); // All normal log producers are now quiescent.
        bridgeBegin();maintenance=true;
        Serial.println("PIUU BRIDGE READY");Serial.flush();
        return true;
    }
    return false;
}


void setup()
{
    Serial.setRxBufferSize(4096);Serial.setTxBufferSize(4096);
    Serial.begin(115200);

    delay(3000);

    Serial.println();
    Serial.println(
        "================================="
    );

    Serial.println(
        " C3 EARS"
    );

    Serial.println(
        "================================="
    );

    Serial.printf(
        "Device: %s | MAC esperada: %s\n",
        DEVICE_NAME,
        DEVICE_MAC
    );


    // ---------------------------------------------
    // Communication
    // ---------------------------------------------

    wakeLinkBegin();

    if (!coreLinkBegin())
    {
        Serial.println("[EARS] Cannot initialize CORE link");
        while (true) delay(1000);
    }


    // Give other controllers time to boot.
    delay(1000);


    // ---------------------------------------------
    // Audio
    // ---------------------------------------------

    if (!audioEngineBegin())
    {
        Serial.println(
            "[EARS] Fatal audio initialization error"
        );

        while (true)
        {
            delay(1000);
        }
    }

    Serial.println(
        "[EARS] Ready"
    );
}


void loop()
{
    if(pollMaintenance())return;
    /*
     * Process control information arriving
     * asynchronously from WAKE.
     */
    wakeLinkProcessRx();

    coreLinkProcessRx();
    UartDiagnostics::poll();

    delay(1);
}