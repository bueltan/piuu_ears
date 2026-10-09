#include "audio_source.h"

#include "audio_source_file.h"
#include "config.h"


bool audioSourceBegin()
{
    switch (AUDIO_SOURCE)
    {
        case AudioSourceType::FILE_REPLAY:

            Serial.println(
                "[EARS] Audio source: FILE_REPLAY"
            );

            return fileAudioSourceBegin();


        case AudioSourceType::MICROPHONE:

            Serial.println(
                "[EARS] Audio source: MICROPHONE "
                "(not implemented yet)"
            );

            return false;
    }

    return false;
}


bool audioSourceReadFrame(
    uint8_t *buffer,
    size_t bufferSize
)
{
    switch (AUDIO_SOURCE)
    {
        case AudioSourceType::FILE_REPLAY:

            return fileAudioSourceReadFrame(
                buffer,
                bufferSize
            );


        case AudioSourceType::MICROPHONE:

            /*
             * Futuro:
             *
             * return microphoneAudioSourceReadFrame(
             *     buffer,
             *     bufferSize
             * );
             */

            return false;
    }

    return false;
}