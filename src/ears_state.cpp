#include "ears_state.h"

#include <atomic>


static std::atomic_bool streamToCore{false};


bool earsIsStreamingToCore()
{
    return streamToCore.load();
}


void earsStartCoreStream()
{
    streamToCore.store(true);
}


void earsStopCoreStream()
{
    streamToCore.store(false);
}