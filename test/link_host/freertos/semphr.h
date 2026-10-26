#pragma once
#include "FreeRTOS.h"
inline SemaphoreHandle_t xSemaphoreCreateMutex() { static std::mutex m; return &m; }
inline int xSemaphoreTake(SemaphoreHandle_t m, int) { m->lock(); return pdTRUE; }
inline void xSemaphoreGive(SemaphoreHandle_t m) { m->unlock(); }
