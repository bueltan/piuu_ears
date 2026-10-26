#pragma once
#include <stdint.h>
#ifndef EARS_UART_DIAGNOSTICS
#define EARS_UART_DIAGNOSTICS 1
#endif
namespace UartDiagnostics {
#if EARS_UART_DIAGNOSTICS
void frame(uint32_t sequence, uint32_t readyUs, uint32_t wakeUs, uint32_t coreUs,
           uint32_t routeUs, bool coreAttempted, bool wakeOk, bool coreOk);
void sourcePaused();
void poll();
#else
inline void sourcePaused() {}
inline void poll() {}
#endif
}
