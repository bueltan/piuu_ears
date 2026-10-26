#pragma once
#include <Arduino.h>
#include "control_protocol.h"

bool coreLinkBegin();
void coreLinkProcessRx();
bool coreLinkSendAudio(uint32_t sequence, const uint8_t *pcm, size_t payloadBytes);
// Forwards the original event and payload. WAKE enables audio under the same
// TX lock after the event, including repeated wake events during streaming.
bool coreLinkForwardEvent(uint8_t event, const uint8_t *payload, size_t payloadBytes);
