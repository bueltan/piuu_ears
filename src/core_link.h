#pragma once
#include <Arduino.h>
#include "control_protocol.h"

bool coreLinkBegin();
void coreLinkProcessRx();
bool coreLinkSendAudio(uint32_t sequence, const uint8_t *pcm, size_t payloadBytes);
// Forwards the original event and payload. An initial WAKE enables both routes
// under the TX lock after the event; repeated WAKE preserves the current routing.
bool coreLinkForwardEvent(uint8_t event, const uint8_t *payload, size_t payloadBytes);

void coreLinkPause();
void coreLinkResume();
