#include <Arduino.h>
#include "config.h"
#include "bridge/protocol.h"
#ifdef EARS_INTEGRATED_BRIDGE
#include "bridge/runtime.h"
static bool exiting=false;
static uint32_t exitAt=0;
#endif

// Shared transport: dedicated firmware or exclusive ownership after normal tasks park.
static HardwareSerial core(0);
static Bridge::Parser parser;
static uint8_t usbTx[Bridge::MaxPayload+7], uartTx[Bridge::MaxPayload];
static size_t usbSize=0,usbSent=0,uartSize=0,uartSent=0;
static bool active=false,uartAckPending=false;
static uint32_t lastUsbByte=0;

static void reply(uint8_t type,const uint8_t *data,size_t n) {
    usbSize=Bridge::encode(type,data,n,usbTx);usbSent=0;
}
static void status(uint8_t type,uint8_t value) {reply(type,&value,1);}
static void releasePin(int pin) {pinMode(pin,OUTPUT_OPEN_DRAIN);digitalWrite(pin,HIGH);}
static void resetCore(bool download) {
    if(active)core.end();
    active=false;
    digitalWrite(CORE_EN_PIN,LOW);
    digitalWrite(CORE_BOOT_PIN,download?LOW:HIGH);
    delay(100);
    if(download){core.setRxBufferSize(4096);core.begin(115200,SERIAL_8N1,CORE_RX_PIN,CORE_TX_PIN);}
    digitalWrite(CORE_EN_PIN,HIGH);
    delay(100); // Strap sampling completes before releasing BOOT.
    digitalWrite(CORE_BOOT_PIN,HIGH);
    // Preserve ROM boot output: it reveals download mode, wrong straps or a missing RX path.
    if(download)active=true;
}
static void command() {
    const auto type=parser.type();const size_t n=parser.size();
    if(type==Bridge::Data && active && n) {
        memcpy(uartTx,parser.data(),n);uartSize=n;uartSent=0;uartAckPending=true;return;
    }
#ifdef EARS_INTEGRATED_BRIDGE
    if(type==Bridge::Exit && !n) {
        if(active){core.end();active=false;}
        status(Bridge::Ack,type);exiting=true;exitAt=0;return;
    }
    if(type==Bridge::Ping && !n) {
        const uint8_t capabilities[]={Bridge::Ping,1};
        reply(Bridge::Ack,capabilities,sizeof(capabilities));return;
    }
#endif
    if(n || (type!=Bridge::Ping && type!=Bridge::Boot && type!=Bridge::Run)) {
        status(Bridge::Error,type);return;
    }
    if(type==Bridge::Boot || type==Bridge::Run)resetCore(type==Bridge::Boot);
    status(Bridge::Ack,type);
}
#ifdef EARS_INTEGRATED_BRIDGE
void bridgeBegin() {
    parser.clear();usbSize=usbSent=uartSize=uartSent=0;
    active=uartAckPending=exiting=false;exitAt=0;
#else
void setup() {
#endif
    releasePin(CORE_BOOT_PIN);releasePin(CORE_EN_PIN);
#ifndef EARS_INTEGRATED_BRIDGE
    Serial.setRxBufferSize(4096);Serial.setTxBufferSize(4096);Serial.begin(115200);
#endif
    // No startup banner or application logs on this binary transport.
}
#ifdef EARS_INTEGRATED_BRIDGE
void bridgePoll() {
#else
void loop() {
#endif
    if(usbSent<usbSize) {
        const size_t room=Serial.availableForWrite();
        if(room)usbSent+=Serial.write(usbTx+usbSent,min(room,usbSize-usbSent));
    }
#ifdef EARS_INTEGRATED_BRIDGE
    if(exiting) {
        if(usbSent==usbSize) {
            if(!exitAt){Serial.flush();exitAt=millis();}
            if(uint32_t(millis()-exitAt)>=250){exiting=false;maintenanceResume();}
        }
        delay(1);return;
    }
#endif
    if(uartSent<uartSize) {
        const size_t room=core.availableForWrite();
        if(room)uartSent+=core.write(uartTx+uartSent,min(room,uartSize-uartSent));
    }
    if(usbSent==usbSize && uartSent==uartSize && uartAckPending) {
        status(Bridge::Ack,Bridge::Data);uartAckPending=false;
    }
    if(usbSent==usbSize && uartSent==uartSize) {
        if(uint32_t(millis()-lastUsbByte)>1000)parser.clear();
        for(unsigned i=0;i<512 && Serial.available();++i) {
            lastUsbByte=millis();
            const int result=parser.feed(uint8_t(Serial.read()));
            if(result<0){status(Bridge::Error,0);break;}
            if(result>0){command();break;}
        }
    }
    if(active && usbSent==usbSize && core.available()) {
        uint8_t data[Bridge::MaxPayload];size_t n=0;
        while(n<sizeof(data) && core.available())data[n++]=uint8_t(core.read());
        reply(Bridge::Data,data,n);
    }
    delay(1);
}
