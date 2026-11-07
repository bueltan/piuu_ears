#pragma once
#include <stdint.h>
constexpr uint8_t EARS_ROUTE_CORE=1, EARS_ROUTE_KEYWORD=2;
uint8_t earsAudioRoutes();
bool earsIsStreamingToCore();
bool earsKeywordsEnabled();
// Wake starts both routes. Legacy START explicitly requests CORE only.
void earsStartCoreStream(bool keywords=true);
void earsStopCoreStream();
