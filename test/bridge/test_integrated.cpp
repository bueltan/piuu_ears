#include <string>
#define EARS_INTEGRATED_BRIDGE 1
#include "bridge/main.cpp"
#include "maintenance_request.h"
#include <cstdio>
static int resumes=0;
void maintenanceResume(){++resumes;assert(usbSent==usbSize && !active && !core.started);}
static void send(uint8_t type){uint8_t packet[16];const size_t n=Bridge::encode(type,nullptr,0,packet);Serial.rx.insert(Serial.rx.end(),packet,packet+n);}
int main(){
    MaintenanceRequest request;
    for(char c:std::string("garbagePIUU BRIDGE\n"))assert(!request.feed(c,now++));
    bool entered=false;
    for(char c:std::string("PIUU BRIDGE\r\n"))entered=request.feed(c,now++);
    assert(entered);
    for(char c:std::string("PIUU "))assert(!request.feed(c,now++));
    now+=1001;
    for(char c:std::string("BRIDGE\n"))assert(!request.feed(c,now++));
    bridgeBegin();send(Bridge::Ping);
    for(int i=0;i<100;++i)bridgePoll();
    Bridge::Parser decoder;bool ack=false;
    for(auto b:Serial.tx)if(decoder.feed(b)==1){assert(decoder.type()==Bridge::Ack && decoder.size()==2 && decoder.data()[1]==1);ack=true;}
    assert(ack);Serial.tx.clear();
    send(Bridge::Boot);for(int i=0;i<100;++i){bridgePoll();}assert(active);
    // Disconnect/idle never leaves maintenance or resets the target automatically.
    const auto writes=pins.size();now+=100000;
    for(int i=0;i<100;++i){bridgePoll();}assert(active && resumes==0 && pins.size()==writes);
    send(Bridge::Run);for(int i=0;i<100;++i){bridgePoll();}assert(!active && resumes==0);
    Serial.tx.clear();send(Bridge::Exit);
    for(int i=0;i<50;++i){bridgePoll();}assert(resumes==0);
    ack=false;
    for(auto b:Serial.tx)if(decoder.feed(b)==1){assert(decoder.type()==Bridge::Ack && decoder.data()[0]==Bridge::Exit);ack=true;}
    assert(ack);
    for(int i=0;i<300;++i){bridgePoll();}assert(resumes==1);
    bridgeBegin();assert(!active && !exiting); // Repeat entry is clean.
    puts("PASS: maintenance line, capabilities, explicit exit, drained ACK and no timeout reset");
}
