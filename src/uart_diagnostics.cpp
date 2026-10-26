#include "uart_diagnostics.h"
#if EARS_UART_DIAGNOSTICS
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include "config.h"
namespace UartDiagnostics {
namespace {
struct Timing {
    uint32_t n=0, max=0, over=0;
    uint64_t total=0;
    void add(uint32_t us) { ++n;total+=us;if(us>max)max=us;if(us>FRAME_MS*1000)++over; }
};
struct Window {
    Timing wake,core,route;
    uint32_t frames=0,first=0,last=0,wakeErrors=0,coreErrors=0;
    uint32_t periods=0,late=0,early=0,maxPeriod=0;
};
portMUX_TYPE lock=portMUX_INITIALIZER_UNLOCKED;
Window window;
// Only audioEngine touches the pacing reference; EOF pauses are excluded.
bool havePrevious=false;
uint32_t previousUs=0;
}
void sourcePaused() { havePrevious=false; }
void frame(uint32_t sequence,uint32_t readyUs,uint32_t wakeUs,uint32_t coreUs,
           uint32_t routeUs,bool coreAttempted,bool wakeOk,bool coreOk) {
    const uint32_t period=readyUs-previousUs;
    portENTER_CRITICAL(&lock);
    if(!window.frames)window.first=sequence;
    ++window.frames;window.last=sequence;
    window.wake.add(wakeUs);window.route.add(routeUs);
    if(coreAttempted)window.core.add(coreUs);
    if(!wakeOk)++window.wakeErrors;
    if(coreAttempted && !coreOk)++window.coreErrors;
    if(havePrevious) {
        ++window.periods;
        if(period>window.maxPeriod)window.maxPeriod=period;
        if(period>FRAME_MS*1000+2000)++window.late;
        if(period<FRAME_MS*1000-2000)++window.early;
    }
    portEXIT_CRITICAL(&lock);
    previousUs=readyUs;havePrevious=true;
}
void poll() {
    static uint32_t lastReport=0;
    const uint32_t now=millis();
    if(uint32_t(now-lastReport)<5000)return;
    lastReport=now;
    Window report;
    portENTER_CRITICAL(&lock);
    report=window;window=Window{};
    portEXIT_CRITICAL(&lock);
    // USB formatting only in loop(), never in the audio task or lock.
    Serial.printf("[UART_TX t=%lu] frames=%lu seq_first=%lu seq_last=%lu wake_n=%lu wake_avg_us=%lu wake_max_us=%lu wake_over20=%lu wake_err=%lu core_n=%lu core_avg_us=%lu core_max_us=%lu core_over20=%lu core_err=%lu route_avg_us=%lu route_max_us=%lu route_over20=%lu\n",
        (unsigned long)now,(unsigned long)report.frames,(unsigned long)report.first,(unsigned long)report.last,
        (unsigned long)report.wake.n,(unsigned long)(report.wake.n?report.wake.total/report.wake.n:0),
        (unsigned long)report.wake.max,(unsigned long)report.wake.over,(unsigned long)report.wakeErrors,
        (unsigned long)report.core.n,(unsigned long)(report.core.n?report.core.total/report.core.n:0),
        (unsigned long)report.core.max,(unsigned long)report.core.over,(unsigned long)report.coreErrors,
        (unsigned long)(report.route.n?report.route.total/report.route.n:0),
        (unsigned long)report.route.max,(unsigned long)report.route.over);
    Serial.printf("[AUDIO_PACING t=%lu] periods=%lu late_gt22ms=%lu early_lt18ms=%lu max_period_us=%lu\n",
        (unsigned long)now,(unsigned long)report.periods,(unsigned long)report.late,
        (unsigned long)report.early,(unsigned long)report.maxPeriod);
}
}
#endif
