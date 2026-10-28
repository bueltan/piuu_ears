#include "microphone_startup.h"
#include <cassert>
#include <cstdio>
int main() {
    MicrophoneStartup gate;gate.reset(16000,1000);
    for(int i=0;i<50;++i)assert(gate.consumeFrame(320));
    assert(gate.samples()==16000 && !gate.consumeFrame(320));
    gate.reset(16000,0);assert(!gate.consumeFrame(320));
    gate.reset(16000,1000);assert(gate.consumeFrame(320));
    // No consume on failed reads: time spent blocked must not expire warmup.
    assert(gate.samples()==320);
    for(int i=1;i<50;++i)assert(gate.consumeFrame(320));
    assert(!gate.consumeFrame(320));
    puts("PASS: sample-paced microphone warmup, disabled mode and reinitialization");
}
