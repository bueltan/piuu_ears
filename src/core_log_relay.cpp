#include "core_log_relay.h"
#include "ears_log.h"
#if !EARS_LOGS_ENABLED
void coreLogRelayReset() {}
void coreLogRelayReceive(const uint8_t*,size_t) {}
void coreLogRelayPoll() {}
#else
#include "core_log_wire.h"
#include <Arduino.h>
#include <cstdio>
#include <cstring>
namespace {
struct Frame { uint16_t size;uint8_t data[CoreLogWire::MaxPayload]; };
Frame queue[8];
size_t head=0,count=0;
uint32_t usbDropped=0,bad=0;
}
void coreLogRelayReset() { head=count=0;usbDropped=bad=0; }
void coreLogRelayReceive(const uint8_t *payload,size_t size) {
    // Only loop owns this queue. Neither CRC errors nor logs alter audio state.
    if(!CoreLogWire::valid(payload,size)) { ++bad;return; }
    if(count==8) { head=(head+1)%8;--count;++usbDropped; }
    auto &frame=queue[(head+count)%8];frame.size=size;
    memcpy(frame.data,payload,size);++count;
}
void coreLogRelayPoll() {
    if(!count)return;
    const auto &frame=queue[head];const auto *p=frame.data;
    char line[384];
    const int prefix=snprintf(line,sizeof(line),
        "[CORE boot=%08lx seq=%lu t=%lu reset=%u lost_bytes=%lu usb_drop=%lu bad_crc=%lu] ",
        (unsigned long)CoreLogWire::get32(p),(unsigned long)CoreLogWire::get32(p+4),
        (unsigned long)CoreLogWire::get32(p+8),unsigned(p[16]),
        (unsigned long)CoreLogWire::get32(p+12),(unsigned long)usbDropped,(unsigned long)bad);
    size_t n=prefix;
    // One complete USB line per chunk. Sanitize control bytes, including newlines,
    // so CORE cannot inject USB maintenance responses or fake EARS log prefixes.
    for(size_t i=CoreLogWire::Metadata;i<size_t(frame.size)-2;++i) {
        const uint8_t c=p[i];line[n++]=(c>=32 && c!=127)?char(c):' ';
    }
    line[n++]='\n';
    // No wait/flush: a missing or slow USB host must not stall audio/control RX.
    if(!Serial || Serial.availableForWrite()<int(n))return;
    if(Serial.write(reinterpret_cast<const uint8_t*>(line),n)!=n)++usbDropped;
    head=(head+1)%8;--count;
}
#endif
