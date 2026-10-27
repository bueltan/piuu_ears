#include "bridge/main.cpp"
#include <cstdio>
static void pump(){for(int i=0;i<200;++i)loop();}
static void send(uint8_t kind,const uint8_t *p=nullptr,size_t n=0,bool corrupt=false){
    uint8_t b[263];size_t size=Bridge::encode(kind,p,n,b);
    if(corrupt)b[size-1]^=1;
    Serial.rx.insert(Serial.rx.end(),b,b+size);pump();
}
static void response(uint8_t kind,const std::vector<uint8_t>& payload){
    Bridge::Parser p;int done=0;
    for(auto b:Serial.tx) {int r=p.feed(b);assert(r>=0);if(r){++done;assert(p.type()==kind);assert(std::vector<uint8_t>(p.data(),p.data()+p.size())==payload);}}
    assert(done==1);Serial.tx.clear();
}
int main(){
    setup();assert(!active && !core.started && Serial.tx.empty());
    send(Bridge::Ping);response(Bridge::Ack,{Bridge::Ping});assert(!core.started);
    const size_t before=pins.size();send(Bridge::Boot);response(Bridge::Ack,{Bridge::Boot});
    assert(active && core.baud==115200 && core.rxPin==7 && core.txPin==6);
    assert(pins.size()==before+4);
    assert(std::get<0>(pins[before])==10 && std::get<1>(pins[before])==LOW);
    assert(std::get<0>(pins[before+1])==2 && std::get<1>(pins[before+1])==LOW);
    assert(std::get<1>(pins[before+2])==HIGH && std::get<1>(pins[before+3])==HIGH);
    assert(std::get<2>(pins[before+2])-std::get<2>(pins[before])>=100);
    std::vector<uint8_t> all;for(unsigned i=0;i<256;++i)all.push_back(i);
    send(Bridge::Data,all.data(),all.size());assert(core.tx==all);response(Bridge::Ack,{Bridge::Data});
    core.rx.insert(core.rx.end(),all.begin(),all.end());pump();response(Bridge::Data,all);
    core.tx.clear();send(Bridge::Data,all.data(),all.size(),true);response(Bridge::Error,{0});assert(core.tx.empty());
    send(Bridge::Run);response(Bridge::Ack,{Bridge::Run});assert(!active && !core.started);
    send(Bridge::Data,all.data(),all.size());response(Bridge::Error,{Bridge::Data});assert(core.tx.empty());
    // A ROM banner already buffered during reset must survive the BOOT ACK.
    const std::vector<uint8_t> banner={'R','O','M'};
    core.rx.insert(core.rx.end(),banner.begin(),banner.end());
    send(Bridge::Boot);
    Bridge::Parser bootParser;unsigned replies=0;
    for(auto b:Serial.tx)if(bootParser.feed(b)==1){
        if(replies++==0)assert(bootParser.type()==Bridge::Ack);
        else {assert(bootParser.type()==Bridge::Data);assert(std::vector<uint8_t>(bootParser.data(),bootParser.data()+bootParser.size())==banner);}
    }
    assert(replies==2);Serial.tx.clear();
    uint8_t encoded[263];size_t n=Bridge::encode(Bridge::Data,all.data(),all.size(),encoded);
    FILE *file=fopen("/tmp/piuu-bridge-golden.bin","wb");assert(file);fwrite(encoded,1,n,file);fclose(file);
    puts("PASS: bridge reset sequence, binary forwarding, partial writes, CRC rejection and maintenance isolation");
}
