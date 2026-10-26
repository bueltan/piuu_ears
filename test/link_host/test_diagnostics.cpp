#include "Arduino.h"
#include "uart_diagnostics.h"
#include <cassert>
SerialData serialData[2];
Logger Serial;
static uint32_t now=0;
uint32_t millis() { return now; }
int main() {
#if EARS_UART_DIAGNOSTICS
    using UartDiagnostics::frame;
    frame(0,20000,3,7,10000,true,true,true);
    frame(1,40000,4,8,10000,true,true,false);
    frame(2,64000,5,0,23000,false,false,true);
    frame(3,80000,6,0,10000,false,true,true);
    UartDiagnostics::sourcePaused();
    frame(4,60000000,7,0,10000,false,true,true); // EOF pause is not jitter.
    now=4999;UartDiagnostics::poll();assert(Serial.lines.empty());
    now=5000;UartDiagnostics::poll();assert(Serial.lines.size()==2);
    assert(Serial.lines[0].find("frames=5 seq_first=0 seq_last=4")!=std::string::npos);
    assert(Serial.lines[0].find("wake_n=5 wake_avg_us=5 wake_max_us=7")!=std::string::npos);
    assert(Serial.lines[0].find("core_n=2 core_avg_us=7 core_max_us=8 core_over20=0 core_err=1")!=std::string::npos);
    assert(Serial.lines[0].find("route_over20=1")!=std::string::npos);
    assert(Serial.lines[1].find("periods=3 late_gt22ms=1 early_lt18ms=1 max_period_us=24000")!=std::string::npos);
    // uint32 micros rollover remains an ordinary 20 ms interval.
    UartDiagnostics::sourcePaused();
    frame(5,UINT32_MAX-9999,1,0,1,false,true,true);
    frame(6,10000,1,0,1,false,true,true);
    now=10000;UartDiagnostics::poll();assert(Serial.lines.size()==4);
    assert(Serial.lines[3].find("periods=1 late_gt22ms=0 early_lt18ms=0 max_period_us=20000")!=std::string::npos);
#else
    UartDiagnostics::sourcePaused();UartDiagnostics::poll();assert(Serial.lines.empty());
#endif
    puts("PASS: bounded timing windows, pacing pause/rollover and diagnostics switch");
}
