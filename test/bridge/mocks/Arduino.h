#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include <deque>
#include <vector>
#include <tuple>
#include <cassert>
using std::min;
constexpr int HIGH=1,LOW=0,OUTPUT_OPEN_DRAIN=3,SERIAL_8N1=0;
static uint32_t now=0;
static std::vector<std::tuple<int,int,uint32_t>> pins;
inline uint32_t millis(){return now;}
inline void delay(unsigned ms){now+=ms;}
inline void digitalWrite(int pin,int value){pins.emplace_back(pin,value,now);}
inline void pinMode(int,int mode){assert(mode==OUTPUT_OPEN_DRAIN);}
class HardwareSerial {
public:
    std::deque<uint8_t> rx;
    std::vector<uint8_t> tx;
    bool started=false;
    unsigned baud=0;
    int rxPin=-1,txPin=-1;
    HardwareSerial(int=0){}
    void begin(unsigned b,int=0,int r=-1,int t=-1){started=true;baud=b;rxPin=r;txPin=t;}
    void flush() {}
    void end(){started=false;rx.clear();}
    void setRxBufferSize(unsigned){}
    void setTxBufferSize(unsigned){}
    int available(){return rx.size();}
    int availableForWrite(){return 7;}
    int read(){if(rx.empty())return -1;auto b=rx.front();rx.pop_front();return b;}
    size_t write(const uint8_t*p,size_t n){assert(n<=7);tx.insert(tx.end(),p,p+n);return n;}
};
static HardwareSerial Serial;
