#include "audio_source.h"

#include "audio_source_file.h"
#include "audio_source_microphone.h"
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
                "[EARS] Audio source: MICROPHONE"
            );

            return microphoneAudioSourceBegin();
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

            return microphoneAudioSourceReadFrame(buffer, bufferSize);
    }

    return false;
}

void audioSourcePause() {
    if(AUDIO_SOURCE==AudioSourceType::MICROPHONE)microphoneAudioSourceEnd();
}
bool audioSourceResume() {
    if(AUDIO_SOURCE==AudioSourceType::MICROPHONE)return microphoneAudioSourceBegin();
    fileAudioSourceResume();return true;
}
