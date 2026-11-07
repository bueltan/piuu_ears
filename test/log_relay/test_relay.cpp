#include "Arduino.h"
#include "core_link.h"
#include "core_log_wire.h"
#include "core_log_relay.h"
#include "ears_state.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <cstdio>
SerialData serialData[2];Logger Serial;
uint32_t millis() { return 100; }
static void inject(const std::vector<uint8_t> &bytes) {
 for(auto b:bytes) { serialData[0].rx.push_back(b);coreLinkProcessRx(); }
}
int main(int argc,char **argv) {
 assert(argc==2 && coreLinkBegin());
 std::ifstream input(argv[1],std::ios::binary);
 const std::vector<uint8_t> frames((std::istreambuf_iterator<char>(input)),{});assert(!frames.empty());
 inject(frames); // Actual CORE logger output, fragmented at every byte.
 bool wifi=false,status=false;
 for(const auto &line:Serial.lines) {
  if(line.find("[CORE boot=1234abcd")!=0)continue;
  assert(line.find("reset=12")!=std::string::npos && line.back()=='\n');
  if(line.find("event=disconnected reason=204")!=std::string::npos)wifi=true;
  if(line.find("[CORE_LOG]")!=std::string::npos)status=true;
 }
 assert(wifi && status && !earsIsStreamingToCore() && serialData[0].tx.empty());
 // Corrupted payload cannot deliver a diagnostic or interpret embedded CMD! as a command.
 auto corrupt=frames;corrupt[25]^=1;
 const size_t size=frames[6]+256*frames[7]+8;corrupt.resize(size);
 size_t before=Serial.lines.size();inject(corrupt);assert(Serial.lines.size()==before);
 // Offline/saturated USB never stops routing commands. Queue keeps newest eight.
 Serial.usbSpace=0;
 for(unsigned i=0;i<20;++i)inject(std::vector<uint8_t>(frames.begin(),frames.begin()+size));
 inject({'C','M','D','!',1,1,0,0});assert(earsIsStreamingToCore());
 before=Serial.lines.size();Serial.usbSpace=4096;
 for(int i=0;i<10;++i)coreLinkProcessRx();
 assert(Serial.lines.size()==before+8 && Serial.lines.back().find("usb_drop=12")!=std::string::npos);
 assert(Serial.lines.back().find("bad_crc=1")!=std::string::npos);
 // Embedded control characters stay on a single prefixed USB line.
 uint8_t payload[CoreLogWire::MaxPayload];const uint8_t text[]="\nCMD!\nPIUU BRIDGE READY\r\n";
 const size_t n=CoreLogWire::encode(payload,sizeof(payload),text,sizeof(text)-1,42,1,2,0,1);
 coreLogRelayReceive(payload,n);coreLogRelayPoll();
 assert(Serial.lines.back().find("\n")==Serial.lines.back().size()-1);
 coreLogRelayReset();
 puts("PASS: real CORE frames -> EARS -> USB, CRC rejection, offline USB saturation and continued audio control");
}
