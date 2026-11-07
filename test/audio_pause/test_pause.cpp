#define EARS_UART_DIAGNOSTICS 0
#include "audio_engine.cpp"
#include <cassert>
#include <vector>
static std::atomic<unsigned> reads{0},wakeFrames{0},coreFrames{0};
static std::vector<uint32_t> wakeSequences,coreSequences;
bool audioSourceBegin(){return true;}
bool audioSourceReadFrame(uint8_t*,size_t){vTaskDelay(2);++reads;return true;}
bool wakeLinkSendAudio(uint32_t seq,const uint8_t*,size_t){wakeSequences.push_back(seq);++wakeFrames;delay(2);return true;}
bool coreLinkSendAudio(uint32_t seq,const uint8_t*,size_t){coreSequences.push_back(seq);++coreFrames;return true;}
static void waitCore(unsigned target) {
    const uint32_t started=millis();
    while(coreFrames.load()<target){assert(uint32_t(millis()-started)<3000);delay(1);}
}
int main(){
    assert(audioEngineBegin());
    while(reads.load()<3)delay(1);
    for(int i=0;i<3;++i){
        assert(audioEnginePause());
        const auto n=reads.load();assert(wakeFrames.load()==n && coreFrames.load()==0);
        delay(20);assert(reads.load()==n && wakeFrames.load()==n && coreFrames.load()==0);
        audioEngineResume();while(reads.load()==n)delay(1);
    }
    assert(audioEnginePause());
    auto wakeBefore=wakeFrames.load(),coreBefore=coreFrames.load();
    earsStartCoreStream(true); // Listening/waiting: both receive identical frames.
    audioEngineResume();waitCore(coreBefore+3);assert(audioEnginePause());
    auto added=coreFrames.load()-coreBefore;
    assert(wakeFrames.load()-wakeBefore==added);
    for(unsigned i=0;i<added;++i) assert(wakeSequences[wakeBefore+i]==coreSequences[coreBefore+i]);
    wakeBefore=wakeFrames.load();coreBefore=coreFrames.load();
    earsStartCoreStream(false); // Speaker playback: CORE still receives PCM.
    audioEngineResume();waitCore(coreBefore+3);assert(audioEnginePause());
    assert(wakeFrames.load()==wakeBefore);
    coreBefore=coreFrames.load();
    earsStartCoreStream(true); // Playback/echo guard ended.
    audioEngineResume();waitCore(coreBefore+3);assert(audioEnginePause());
    assert(wakeFrames.load()>wakeBefore);
    coreBefore=coreFrames.load();wakeBefore=wakeFrames.load();
    earsStopCoreStream(); // Idle returns to keyword-only routing.
    audioEngineResume();while(wakeFrames.load()<wakeBefore+3)delay(1);assert(audioEnginePause());
    assert(coreFrames.load()==coreBefore);
    finish.store(true);worker.join();
    puts("PASS: cooperative pause, concurrent identical PCM, speaker-only keyword suppression and idle/resume routing");
}
