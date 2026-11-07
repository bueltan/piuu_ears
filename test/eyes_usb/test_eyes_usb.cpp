#include "eyes_usb_command.h"
#include "maintenance_request.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main(){
    EyesUsbCommand parser;MaintenanceRequest maintenance;unsigned commands=0,invalid=0,bridges=0;
    uint32_t now=0xfffffff0;
    auto feed=[&](const char* text){for(;*text;++text){auto result=parser.feed(*text,now);commands+=result==EyesUsbCommand::Command;invalid+=result==EyesUsbCommand::Invalid;bridges+=maintenance.feed(*text,now++);}};
    feed("cell 3\nEYES cell 3\r\n");assert(commands==1 && !strcmp(parser.command,"cell 3"));
    feed("EYES activity speaking\n");assert(commands==2 && !strcmp(parser.command,"activity speaking"));
    feed("EYES \n");assert(invalid==1);
    feed("EYES 1234567890123456789012345678901\n");assert(commands==3 && strlen(parser.command)==31);
    feed("EYES 12345678901234567890123456789012\n");assert(invalid==2);
    feed("EYES a\tb\n");assert(invalid==3);
    feed("EYES battery ");now+=1001;feed("18\n");assert(commands==3);
    feed("PIUU BRIDGE\n");assert(bridges==1 && commands==3);
    feed("EYES volume 60\n");assert(commands==4);
    puts("PASS: EYES USB prefix, bounds, CRLF, timeout rollover and maintenance isolation");
}
