#include "ears_state.h"
#include <atomic>
namespace { std::atomic<uint8_t> routes{EARS_ROUTE_KEYWORD}; }
uint8_t earsAudioRoutes() { return routes.load(); }
bool earsIsStreamingToCore() { return (earsAudioRoutes()&EARS_ROUTE_CORE)!=0; }
bool earsKeywordsEnabled() { return (earsAudioRoutes()&EARS_ROUTE_KEYWORD)!=0; }
void earsStartCoreStream(bool keywords) {
    routes.store(EARS_ROUTE_CORE | (keywords ? EARS_ROUTE_KEYWORD : 0));
}
void earsStopCoreStream() { routes.store(EARS_ROUTE_KEYWORD); }
