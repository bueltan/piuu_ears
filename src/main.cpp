#include <Arduino.h>

#include "wake_link.h"
#include "core_link.h"
#include "audio_engine.h"
#include "uart_diagnostics.h"


void setup()
{
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
    /*
     * Process control information arriving
     * asynchronously from WAKE.
     */
    wakeLinkProcessRx();

    coreLinkProcessRx();
    UartDiagnostics::poll();

    delay(1);
}