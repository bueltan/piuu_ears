#include "ears_log.h"
#include "audio_source_file.h"

#include "config.h"

#include <Arduino.h>
#include <LittleFS.h>


static constexpr char AUDIO_FILE[] = "/test.raw";

static File audioFile;

static TickType_t lastWakeTime;

static const TickType_t framePeriod =
    pdMS_TO_TICKS(FRAME_MS);


// ============================================================
// File helpers
// ============================================================

static bool openAudioFile()
{
    if (audioFile)
    {
        audioFile.close();
    }

    audioFile = LittleFS.open(
        AUDIO_FILE,
        "r"
    );

    if (!audioFile)
    {
        EARS_LOGF(
            "[EARS] ERROR opening %s\n",
            AUDIO_FILE
        );

        return false;
    }

    EARS_LOGF(
        "[EARS] Opened %s size=%u bytes\n",
        AUDIO_FILE,
        static_cast<unsigned>(
            audioFile.size()
        )
    );

    return true;
}


// ============================================================
// Public initialization
// ============================================================

bool fileAudioSourceBegin()
{
    EARS_LOGLN(
        "[EARS] Mounting LittleFS..."
    );

    if (!LittleFS.begin(false))
    {
        EARS_LOGLN(
            "[EARS] ERROR mounting LittleFS"
        );

        return false;
    }

    EARS_LOGLN(
        "[EARS] LittleFS OK"
    );

    if (!openAudioFile())
    {
        return false;
    }

    lastWakeTime =
        xTaskGetTickCount();

    EARS_LOGLN(
        "[EARS] ===== AUDIO START ====="
    );

    return true;
}


// ============================================================
// Read one real-time 20 ms audio frame
// ============================================================

bool fileAudioSourceReadFrame(
    uint8_t *buffer,
    size_t bufferSize
)
{
    if (
        buffer == nullptr ||
        bufferSize < FRAME_BYTES
    )
    {
        return false;
    }

    const size_t bytesRead =
        audioFile.read(
            buffer,
            FRAME_BYTES
        );


    // --------------------------------------------------------
    // Complete frame
    // --------------------------------------------------------

    if (bytesRead == FRAME_BYTES)
    {
        vTaskDelayUntil(
            &lastWakeTime,
            framePeriod
        );

        return true;
    }


    // --------------------------------------------------------
    // Partial final frame
    // --------------------------------------------------------

    if (bytesRead > 0)
    {
        /*
         * Fill the missing samples with digital silence
         * so every output frame remains exactly 20 ms.
         */
        memset(
            buffer + bytesRead,
            0,
            FRAME_BYTES - bytesRead
        );

        EARS_LOGF(
            "[EARS] Final partial frame: "
            "%u bytes + silence\n",
            static_cast<unsigned>(
                bytesRead
            )
        );

        vTaskDelayUntil(
            &lastWakeTime,
            framePeriod
        );

        return true;
    }


    // --------------------------------------------------------
    // End of file
    // --------------------------------------------------------

    EARS_LOGLN();

    EARS_LOGLN(
        "[EARS] ===== END OF AUDIO ====="
    );

    EARS_LOGLN(
        "[EARS] Restarting audio in 1 minute..."
    );

    vTaskDelay(
        pdMS_TO_TICKS(60000)
    );

    if (!openAudioFile())
    {
        EARS_LOGLN(
            "[EARS] Cannot reopen audio file"
        );

        /*
         * Avoid hammering LittleFS if reopening fails.
         */
        vTaskDelay(
            pdMS_TO_TICKS(5000)
        );

        return false;
    }

    /*
     * Reset only the real-time pacing reference.
     *
     * IMPORTANT:
     * sequence is NOT reset here.
     * sequence belongs to audio_engine.cpp.
     */
    lastWakeTime =
        xTaskGetTickCount();

    EARS_LOGLN(
        "[EARS] ===== AUDIO START ====="
    );

    /*
     * This call did not produce an audio frame.
     * audio_engine will call us again.
     */
    return false;
}
void fileAudioSourceResume() { lastWakeTime=xTaskGetTickCount(); }
