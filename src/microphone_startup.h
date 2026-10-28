#pragma once
#include <stdint.h>
#include <stddef.h>

// Count captured samples rather than wall time: queued DMA must also be drained.
class MicrophoneStartup {
public:
    void reset(uint32_t rate, uint32_t muteMs) {
        seen_=0; muteSamples_=uint64_t(rate)*muteMs/1000;
    }
    bool consumeFrame(size_t samples) {
        const bool muted=seen_<muteSamples_;
        seen_+=samples;
        return muted;
    }
    uint64_t samples() const { return seen_; }
private:
    uint64_t seen_=0,muteSamples_=0;
};
