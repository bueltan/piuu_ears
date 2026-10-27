#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
class MaintenanceRequest {
    char line_[24]{};
    size_t used_=0;
    bool overflow_=false;
    uint32_t last_=0;
public:
    bool feed(char c,uint32_t now) {
        if(uint32_t(now-last_)>1000){used_=0;overflow_=false;}
        last_=now;
        if(c=='\r')return false;
        if(c=='\n') {
            line_[used_]=0;
            const bool enter=!overflow_ && used_==11 && strcmp(line_,"PIUU BRIDGE")==0;
            used_=0;overflow_=false;return enter;
        }
        if(used_<sizeof(line_)-1)line_[used_++]=c;else overflow_=true;
        return false;
    }
};
