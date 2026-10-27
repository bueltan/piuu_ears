#pragma once
#include <stdint.h>
#include <stddef.h>
namespace Bridge {
constexpr size_t MaxPayload=256;
enum Type : uint8_t { Ping=1, Boot=2, Run=3, Data=4, Ack=5, Error=6, Exit=7 };
inline uint16_t crc(const uint8_t *p,size_t n) {
    uint16_t c=0xffff;
    while(n--) { c^=uint16_t(*p++)<<8; for(int i=0;i<8;++i)c=(c&0x8000)?uint16_t((c<<1)^0x1021):uint16_t(c<<1); }
    return c;
}
inline size_t encode(uint8_t type,const uint8_t *data,size_t n,uint8_t *out) {
    if(n>MaxPayload)return 0;
    out[0]='E';out[1]='B';out[2]=type;out[3]=n;out[4]=n>>8;
    for(size_t i=0;i<n;++i)out[5+i]=data[i];
    const uint16_t c=crc(out+2,n+3);out[5+n]=c;out[6+n]=c>>8;
    return n+7;
}
class Parser {
    uint8_t bytes_[MaxPayload+7]{};
    size_t used_=0;
public:
    void clear(){used_=0;}
    // 1 complete, -1 malformed, 0 incomplete. Payload valid until next feed.
    int feed(uint8_t b) {
        if(used_==0 && b!='E')return 0;
        if(used_==1 && b!='B'){used_=b=='E'?1:0;return 0;}
        bytes_[used_++]=b;
        if(used_<5)return 0;
        const size_t n=size();
        if(n>MaxPayload){clear();return -1;}
        if(used_<n+7)return 0;
        clear();
        return crc(bytes_+2,n+3)==uint16_t(bytes_[n+5]|uint16_t(bytes_[n+6])<<8)?1:-1;
    }
    uint8_t type()const{return bytes_[2];}
    size_t size()const{return bytes_[3]|size_t(bytes_[4])<<8;}
    const uint8_t *data()const{return bytes_+5;}
};
}
