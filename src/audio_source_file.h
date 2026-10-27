#pragma once

#include <Arduino.h>

bool fileAudioSourceBegin();

bool fileAudioSourceReadFrame(
    uint8_t *buffer,
    size_t bufferSize
);
void fileAudioSourceResume();
