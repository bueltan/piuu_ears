#pragma once
#include <stdint.h>
#include <stddef.h>
void coreLogRelayReceive(const uint8_t *payload,size_t size);
void coreLogRelayPoll();
void coreLogRelayReset();
