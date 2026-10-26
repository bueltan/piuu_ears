#include <assert.h>
#include "microphone_pcm.h"

int main()
{
    assert(microphonePcm16(0, 16) == 0);
    assert(microphonePcm16(INT32_MAX & ~255, 16) == INT16_MAX);
    assert(microphonePcm16(INT32_MIN, 16) == INT16_MIN);
    assert(microphonePcm16(0x12345600, 16) == 0x1234);
    assert(microphonePcm16(-65536, 16) == -1);
    // Increased gain must saturate both signs instead of wrapping as a cast does.
    assert(microphonePcm16(0x40000000, 15) == INT16_MAX);
    assert(microphonePcm16(INT32_MIN, 15) == INT16_MIN);
    assert(microphonePcm16(65536, 15) == 2);
}
