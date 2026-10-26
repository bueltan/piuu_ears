// Compile the actual KEYWORD sender with the actual EARS parser and relay.
#include "Arduino.h"
#include "ears_link.h"
#include "wake_link.h"
#include "core_link.h"
#include "ears_state.h"
#include <cassert>
#include <cstdio>
SerialData serialData[2];
Logger Serial;
static uint32_t now = 0;
uint32_t millis() { return now; }

static void deliverKeywordTx(bool fragmented) {
    const auto bytes = serialData[1].tx;
    serialData[1].tx.clear();
    for (auto b : bytes) {
        serialData[1].rx.push_back(b);
        if (fragmented) { ++now; wakeLinkProcessRx(); }
    }
    wakeLinkProcessRx();
    assert(serialData[0].tx == bytes);
    serialData[0].tx.clear();
}
int main() {
    assert(earsLinkBegin());
    assert(coreLinkBegin());
    wakeLinkBegin();
    assert(!earsIsStreamingToCore());
    assert(earsLinkSendEvent(EarsProtocol::AudioEvent::WAKE_WORD));
    const std::vector<uint8_t> expected = {'E','V','T','!',1,1,0,0};
    assert(serialData[1].tx == expected);
    deliverKeywordTx(true);
    assert(earsIsStreamingToCore());
    // Repeated wake, and every supported event with an opaque maximum payload.
    assert(earsLinkSendEvent(EarsProtocol::AudioEvent::WAKE_WORD));
    deliverKeywordTx(false);
    uint8_t payload[32] = {'E','V','T','!','P','C','M','!'};
    for (unsigned code = 1; code <= 4; ++code) {
        assert(earsLinkSendEvent(static_cast<EarsProtocol::AudioEvent>(code), payload, sizeof(payload)));
        deliverKeywordTx(true);
    }
    assert(!earsLinkSendEvent(EarsProtocol::AudioEvent::NONE));
    assert(!earsLinkSendEvent(EarsProtocol::AudioEvent::WAKE_WORD, nullptr, 1));
    assert(!earsLinkSendEvent(EarsProtocol::AudioEvent::WAKE_WORD, payload, 33));
    assert(serialData[1].tx.empty());
    puts("PASS: actual KEYWORD EVT sender -> EARS parser -> CORE relay, fragmented and repeated wake");
}
