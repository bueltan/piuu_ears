#include "audio_engine.h"

#include "audio_source.h"
#include "wake_link.h"
#include "core_link.h"
#include "ears_state.h"
#include "config.h"
#include "uart_diagnostics.h"

#include <Arduino.h>

static uint8_t audioBuffer[FRAME_BYTES];

static uint32_t sequence = 0;

static uint32_t totalFrames = 0;
static uint32_t totalBytes = 0;


static void audioTask(void *parameter)
{
    (void)parameter;

    Serial.println(
        "[EARS] Audio engine started"
    );

    for (;;)
    {
        const bool frameReady =
            audioSourceReadFrame(
                audioBuffer,
                sizeof(audioBuffer)
            );

        if (!frameReady)
        {
            UartDiagnostics::sourcePaused();
            /*
             * FILE_REPLAY devuelve false cuando llega al EOF,
             * espera el intervalo configurado y vuelve a abrir el archivo.
             *
             * sequence NO se resetea.
             */
            vTaskDelay(
                pdMS_TO_TICKS(1)
            );

            continue;
        }

        // ----------------------------------------------------
        // WAKE siempre recibe audio
        // ----------------------------------------------------

#if EARS_UART_DIAGNOSTICS
        const uint32_t readyUs=micros();
#endif
        const bool wakeOk=wakeLinkSendAudio(sequence,audioBuffer,FRAME_BYTES);
#if EARS_UART_DIAGNOSTICS
        const uint32_t wakeDoneUs=micros();
#endif
        const bool coreAttempted=earsIsStreamingToCore();
        bool coreOk=true;
        if(coreAttempted) {
            coreOk=coreLinkSendAudio(sequence,audioBuffer,FRAME_BYTES);
        }
#if EARS_UART_DIAGNOSTICS
        const uint32_t routeDoneUs=micros();
        UartDiagnostics::frame(sequence,readyUs,wakeDoneUs-readyUs,
            routeDoneUs-wakeDoneUs,routeDoneUs-readyUs,coreAttempted,wakeOk,coreOk);
#endif
        if(!wakeOk) Serial.println("[EARS] WAKE UART TX error");
        if(!coreOk) Serial.println("[EARS] CORE UART TX error");

        // ----------------------------------------------------
        // Este sequence pertenece al frame de audio global.
        // Se incrementa una sola vez después de rutear el frame.
        // ----------------------------------------------------

        sequence++;
        totalFrames++;
        totalBytes += FRAME_BYTES;

        // 50 frames * 20 ms = 1 segundo
        if ((totalFrames % 50) == 0)
        {
            Serial.printf(
                "[EARS] seq=%lu frames=%lu bytes=%lu core=%s\n",
                static_cast<unsigned long>(sequence),
                static_cast<unsigned long>(totalFrames),
                static_cast<unsigned long>(totalBytes),
                earsIsStreamingToCore()
                    ? "ON"
                    : "OFF"
            );
        }
    }
}


bool audioEngineBegin()
{
    if (!audioSourceBegin())
    {
        Serial.println(
            "[EARS] Audio source initialization failed"
        );

        return false;
    }

    const BaseType_t result =
        xTaskCreate(
            audioTask,
            "audioEngine",
            8192,
            nullptr,
            3,
            nullptr
        );

    if (result != pdPASS)
    {
        Serial.println(
            "[EARS] Cannot create audioEngine task"
        );

        return false;
    }

    return true;
}