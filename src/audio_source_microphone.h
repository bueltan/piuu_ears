#pragma once

#include <stddef.h>
#include <stdint.h>

bool microphoneAudioSourceBegin();
bool microphoneAudioSourceReadFrame(uint8_t *buffer, size_t bufferSize);

void microphoneAudioSourceEnd();
