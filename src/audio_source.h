#pragma once

#include <Arduino.h>

bool audioSourceBegin();

bool audioSourceReadFrame(
    uint8_t *buffer,
    size_t bufferSize
);
void audioSourcePause();
bool audioSourceResume();
