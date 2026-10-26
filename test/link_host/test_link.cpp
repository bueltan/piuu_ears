#include "Arduino.h"
#include "core_link.h"
#include "wake_link.h"
#include "ears_state.h"
#include "control_parser.h"
#include <cassert>
#include <thread>
#include <cstdio>
SerialData serialData[2];
Logger Serial;
static uint32_t now = 0;
uint32_t millis() { return now; }

static void inject(int id, const std::vector<uint8_t>& p) {
    serialData[id].rx.insert(serialData[id].rx.end(),p.begin(),p.end());
}
static std::vector<uint8_t> packet(const char *magic, uint8_t code,
                                  std::vector<uint8_t> payload={}, uint8_t version=1) {
    std::vector<uint8_t> p(magic,magic+4);
    p.insert(p.end(),{version,code,uint8_t(payload.size()),uint8_t(payload.size()>>8)});
    p.insert(p.end(),payload.begin(),payload.end()); return p;
}
static void expectState(uint8_t state) {
    assert(serialData[0].tx==packet("EVT!",0x80,{state}));
    serialData[0].tx.clear();
}
static size_t deliveries=0;
static uint8_t lastCode;
static std::vector<uint8_t> lastPayload;
static void receive(uint8_t version, uint8_t code, const uint8_t *p, size_t n) {
    assert(version==1); ++deliveries; lastCode=code;
    lastPayload.clear(); if(n) lastPayload.assign(p,p+n);
}
int main() {
    // Arbitrary fragment boundaries, embedded magic and oversize discard.
    auto event=packet("EVT!",3,{'P','C','M','!','E','V','T','!'});
    for(size_t split=0;split<=event.size();++split) {
        ControlParser parser(EVT_MAGIC,receive);
        const auto before=deliveries;
        for(size_t i=0;i<split;++i) parser.feed(event[i],now);
        parser.expire(now+1,100);
        for(size_t i=split;i<event.size();++i) parser.feed(event[i],now+1);
        assert(deliveries==before+1 && lastCode==3 && lastPayload.size()==8);
    }
    ControlParser parser(EVT_MAGIC,receive);
    auto huge=packet("EVT!",9,std::vector<uint8_t>(33,'E'));
    const auto before=deliveries;
    for(auto b:huge) parser.feed(b,0);
    assert(deliveries==before);
    for(auto b:event) parser.feed(b,0);
    assert(deliveries==before+1);
    for(size_t i=0;i<10;++i) parser.feed(event[i],0); // truncated payload
    parser.expire(101,100);
    for(auto b:event) parser.feed(b,102);
    assert(deliveries==before+2);
    // Clock rollover timeout recovery.
    parser.feed('E',UINT32_MAX-10); parser.expire(100,100);
    for(auto b:event) parser.feed(b,101);
    assert(deliveries==before+3);

    assert(coreLinkBegin()); wakeLinkBegin();
    assert(!earsIsStreamingToCore());
    auto start=packet("CMD!",1);
    // Feed one byte per loop invocation.
    for(auto b:start) { inject(0,{b}); coreLinkProcessRx(); }
    assert(earsIsStreamingToCore()); expectState(1);
    inject(0,start); coreLinkProcessRx(); expectState(1); // idempotent
    inject(0,packet("CMD!",2,{},2)); coreLinkProcessRx(); // wrong version
    inject(0,packet("CMD!",2,{1})); coreLinkProcessRx(); // invalid payload
    inject(0,packet("CMD!",99)); coreLinkProcessRx();
    assert(earsIsStreamingToCore() && serialData[0].tx.empty());
    inject(0,packet("CMD!",2)); coreLinkProcessRx();
    assert(!earsIsStreamingToCore()); expectState(0);
    uint8_t pcm[640]={};
    assert(coreLinkSendAudio(5,pcm,sizeof(pcm)) && serialData[0].tx.empty());
    // Unknown future events relay unchanged while off.
    auto future=packet("EVT!",42,{11,22,33});
    inject(1,future); wakeLinkProcessRx();
    assert(serialData[0].tx==future && !earsIsStreamingToCore()); serialData[0].tx.clear();
    auto wake=packet("EVT!",1,{17});
    inject(1,wake); wakeLinkProcessRx();
    assert(serialData[0].tx==wake && earsIsStreamingToCore()); serialData[0].tx.clear();
    inject(1,wake); wakeLinkProcessRx(); // repeated wake must not disappear
    assert(serialData[0].tx==wake); serialData[0].tx.clear();
    // Concurrent event/audio writers: every packet must remain whole.
    std::thread audio([&] { for(int i=0;i<100;++i) assert(coreLinkSendAudio(i,pcm,sizeof(pcm))); });
    std::thread control([&] { for(int i=0;i<100;++i) assert(coreLinkForwardEvent(42,nullptr,0)); });
    audio.join(); control.join();
    size_t offset=0, audios=0, events=0;
    const auto &tx=serialData[0].tx;
    while(offset<tx.size()) {
        assert(offset+8<=tx.size());
        if(!memcmp(tx.data()+offset,"PCM!",4)) {
            assert(offset+650<=tx.size() && tx[offset+8]==128 && tx[offset+9]==2);
            assert(!memcmp(tx.data()+offset+10,pcm,640)); offset+=650; ++audios;
        } else {
            assert(!memcmp(tx.data()+offset,"EVT!",4) && tx[offset+5]==42 && tx[offset+6]==0);
            offset+=8; ++events;
        }
    }
    assert(audios==100 && events==100);
    puts("PASS: framing, timeout, commands, event relay and concurrent PCM/EVT serialization");
}
