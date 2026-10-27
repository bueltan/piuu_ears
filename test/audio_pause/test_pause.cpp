#define EARS_UART_DIAGNOSTICS 0
#include "audio_engine.cpp"
#include <cassert>
static std::atomic<unsigned> reads{0},wakeFrames{0},coreFrames{0};
bool audioSourceBegin(){return true;}
bool audioSourceReadFrame(uint8_t*,size_t){vTaskDelay(2);++reads;return true;}
bool earsIsStreamingToCore(){return true;}
bool wakeLinkSendAudio(uint32_t,const uint8_t*,size_t){++wakeFrames;delay(2);return true;}
bool coreLinkSendAudio(uint32_t,const uint8_t*,size_t){++coreFrames;return true;}
int main(){
    assert(audioEngineBegin());
    while(reads.load()<3)delay(1);
    for(int i=0;i<3;++i){
        assert(audioEnginePause());
        const auto n=reads.load();assert(wakeFrames.load()==n && coreFrames.load()==n);
        delay(20);assert(reads.load()==n && wakeFrames.load()==n && coreFrames.load()==n);
        audioEngineResume();while(reads.load()==n)delay(1);
    }
    assert(audioEnginePause());finish.store(true);worker.join();
    puts("PASS: cooperative audio pause drains both routes and resumes repeatedly");
}
