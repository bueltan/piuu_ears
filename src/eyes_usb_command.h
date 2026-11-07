#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
// Local USB developer commands only. Ordinary keyword events keep their source.
class EyesUsbCommand {
public:
    enum Result { None, Command, Invalid };
    static constexpr uint8_t EventCode=0x06; // PIUU_EYES debug; not a CORE system event.
    char command[32]{};
    Result feed(char c,uint32_t now) {
        if(uint32_t(now-last_)>1000){used_=0;overflow_=false;}
        last_=now;
        if(c=='\r')return None;
        if(c=='\n'){
            line_[used_]=0;
            const bool prefix=used_>=5 && !memcmp(line_,"EYES ",5);
            Result result=None;
            if(prefix){
                result=overflow_ || used_<=5?Invalid:Command;
                if(result==Command)memcpy(command,line_+5,used_-4);
            }
            used_=0;overflow_=false;return result;
        }
        if(c<32 || c>126)overflow_=true;
        if(used_<sizeof(line_)-1)line_[used_++]=c;else overflow_=true;
        return None;
    }
private:
    char line_[37]{};
    size_t used_=0;
    bool overflow_=false;
    uint32_t last_=0;
};
