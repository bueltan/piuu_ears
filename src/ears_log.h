#pragma once
#include <Arduino.h>
#ifndef EARS_LOGS_ENABLED
#define EARS_LOGS_ENABLED 1
#endif
#if EARS_LOGS_ENABLED
#define EARS_LOGF(...) Serial.printf(__VA_ARGS__)
#define EARS_LOGLN(...) Serial.println(__VA_ARGS__)
#else
#define EARS_LOGF(...) ((void)0)
#define EARS_LOGLN(...) ((void)0)
#endif
