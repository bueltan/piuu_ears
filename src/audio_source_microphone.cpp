#include "ears_log.h"
#include "audio_source_microphone.h"

#include "config.h"
#include "microphone_pcm.h"
#include "microphone_startup.h"
#include <math.h>
#include <driver/i2s.h>
#include <esp_intr_alloc.h>
#include <string.h>

namespace {
constexpr i2s_port_t kMicPort = I2S_NUM_0;
constexpr uint32_t kReadTimeoutMs = 100;
int32_t rawFrame[FRAME_SAMPLES];
bool initialized = false;
MicrophoneStartup startup;
bool wasMuted=false;
#if EARS_MIC_DIAGNOSTICS
QueueHandle_t events = nullptr;
uint32_t frames = 0, errors = 0, overflows = 0, clipped = 0;
uint32_t peak = 0, lastReport = 0;
uint64_t absoluteSum = 0;
uint64_t startupSquares=0;
int64_t startupSum=0;
uint32_t startupPeak=0,startupNearFull=0,startupCount=0,lastNearFullMs=0;

void reportDiagnostics()
{
    i2s_event_t event;
    while (events && xQueueReceive(events, &event, 0) == pdTRUE) {
        if (event.type == I2S_EVENT_RX_Q_OVF) ++overflows;
    }
    const uint32_t now = millis();
    if (now - lastReport < 5000) return;
    EARS_LOGF("[MIC] frames=%lu peak=%lu mean_abs=%lu clipped=%lu read_errors=%lu rx_overflow=%lu\n",
        static_cast<unsigned long>(frames), static_cast<unsigned long>(peak),
        static_cast<unsigned long>(frames ? absoluteSum / (uint64_t(frames) * FRAME_SAMPLES) : 0),
        static_cast<unsigned long>(clipped), static_cast<unsigned long>(errors),
        static_cast<unsigned long>(overflows));
    frames = errors = overflows = clipped = peak = 0;
    absoluteSum = 0;
    lastReport = now;
}
#endif
} // namespace

bool microphoneAudioSourceBegin()
{
    if (initialized) return true;
    i2s_config_t config = {};
    config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX);
    config.sample_rate = SAMPLE_RATE_HZ;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
    config.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT;
    config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    config.dma_buf_count = 8;
    config.dma_buf_len = FRAME_SAMPLES;
    config.use_apll = false;
    config.fixed_mclk = 0;

    i2s_pin_config_t pins = {};
    pins.mck_io_num = I2S_PIN_NO_CHANGE;
    pins.bck_io_num = MIC_BCLK_PIN;
    pins.ws_io_num = MIC_WS_PIN;
    pins.data_out_num = I2S_PIN_NO_CHANGE;
    pins.data_in_num = MIC_DATA_PIN;

#if EARS_MIC_DIAGNOSTICS
    esp_err_t err = i2s_driver_install(kMicPort, &config, 16, &events);
#else
    esp_err_t err = i2s_driver_install(kMicPort, &config, 0, nullptr);
#endif
    if (err != ESP_OK) {
        EARS_LOGF("[MIC] driver_install failed: %s\n", esp_err_to_name(err));
        return false;
    }
    err = i2s_set_pin(kMicPort, &pins);
    if (err != ESP_OK) {
        EARS_LOGF("[MIC] set_pin failed: %s\n", esp_err_to_name(err));
        i2s_driver_uninstall(kMicPort);
        return false;
    }
    startup.reset(SAMPLE_RATE_HZ,EARS_MIC_STARTUP_MUTE_MS);
    wasMuted=EARS_MIC_STARTUP_MUTE_MS>0;
    initialized = true;
#if EARS_MIC_DIAGNOSTICS
    lastReport = millis();
    startupSquares=0;startupSum=0;startupPeak=startupNearFull=startupCount=lastNearFullMs=0;
#endif
    EARS_LOGF("[MIC] INMP441 I2S0 BCLK=%d WS=%d SD=%d, %lu Hz, 32-bit RIGHT -> PCM16, shift=%u\n",
        MIC_BCLK_PIN, MIC_WS_PIN, MIC_DATA_PIN,
        static_cast<unsigned long>(SAMPLE_RATE_HZ), MIC_PCM_SHIFT);
    EARS_LOGF("[MIC] startup_mute_ms=%u pcm_shift=%u gain=%u%%\n",
        unsigned(EARS_MIC_STARTUP_MUTE_MS),
        MIC_PCM_SHIFT,
        unsigned(MIC_GAIN_PERCENT));
    return true;
}

bool microphoneAudioSourceReadFrame(uint8_t *buffer, size_t bufferSize)
{
    if (!initialized || !buffer || bufferSize < FRAME_BYTES) return false;
    size_t offset = 0;
    const uint32_t started = millis();
    while (offset < sizeof(rawFrame)) {
        const uint32_t elapsed = millis() - started;
        if (elapsed >= kReadTimeoutMs) break;
        size_t received = 0;
        const esp_err_t err = i2s_read(kMicPort,
            reinterpret_cast<uint8_t *>(rawFrame) + offset,
            sizeof(rawFrame) - offset, &received,
            pdMS_TO_TICKS(kReadTimeoutMs - elapsed));
        offset += received;
        if (err != ESP_OK || received == 0) break;
    }
    if (offset != sizeof(rawFrame)) {
#if EARS_MIC_DIAGNOSTICS
        ++errors;
        reportDiagnostics();
#endif
        return false;
    }
    const bool muted=startup.consumeFrame(FRAME_SAMPLES);
    for (size_t i = 0; i < FRAME_SAMPLES; ++i) {

        // Convert I2S 32-bit to PCM16
        const int16_t original = microphonePcm16(rawFrame[i], MIC_PCM_SHIFT);

        // Apply configurable digital gain
        int32_t amplified = (int32_t(original) * MIC_GAIN_PERCENT) / 100;

        // Saturate to PCM16 range
        if (amplified > INT16_MAX) amplified = INT16_MAX;
        if (amplified < INT16_MIN) amplified = INT16_MIN;

        const int16_t sample = static_cast<int16_t>(amplified);
        // Explicit little-endian output; do not assume byte-buffer alignment.
        const uint16_t bits = static_cast<uint16_t>(muted ? 0 : sample);
        buffer[i * 2] = static_cast<uint8_t>(bits);
        buffer[i * 2 + 1] = static_cast<uint8_t>(bits >> 8);
#if EARS_MIC_DIAGNOSTICS
        const uint32_t magnitude = sample < 0 ? -int32_t(sample) : sample;
        if (magnitude > peak) peak = magnitude;
        absoluteSum += magnitude;
        if(startup.samples()<=uint64_t(SAMPLE_RATE_HZ)*3) {
            startupSum+=sample;startupSquares+=uint64_t(int64_t(sample)*sample);
            if(magnitude>startupPeak)startupPeak=magnitude;
            if(magnitude>=30000) {
                ++startupNearFull;
                lastNearFullMs=uint32_t((startup.samples()-FRAME_SAMPLES+i)*1000/SAMPLE_RATE_HZ);
            }
            ++startupCount;
        }
        if (sample == INT16_MIN || sample == INT16_MAX) ++clipped;
#endif
    }
#if EARS_MIC_DIAGNOSTICS
    if(startupCount>=SAMPLE_RATE_HZ/10) {
        const uint32_t capturedMs=uint32_t(startup.samples()*1000/SAMPLE_RATE_HZ);
        EARS_LOGF("[MIC_START] end_ms=%lu peak=%lu rms=%.1f dc=%.1f near_full=%lu last_near_full_ms=%lu muted=%d\n",
            (unsigned long)capturedMs,(unsigned long)startupPeak,
            sqrt(double(startupSquares)/startupCount),double(startupSum)/startupCount,
            (unsigned long)startupNearFull,(unsigned long)lastNearFullMs,muted);
        startupSquares=0;startupSum=0;startupPeak=startupNearFull=startupCount=0;
    }
    ++frames;
    reportDiagnostics();
#endif
    if(wasMuted && !muted) {
        EARS_LOGF("[MIC] startup_ready captured_ms=%lu\n",
            (unsigned long)(startup.samples()*1000/SAMPLE_RATE_HZ));
        wasMuted=false;
    }
    // I2S DMA paces live capture. No additional 20 ms task delay is needed.
    return true;
}

// Called only after the audio task has acknowledged its frame-boundary pause.
void microphoneAudioSourceEnd() {
    if(initialized) {i2s_driver_uninstall(kMicPort);initialized=false;}
#if EARS_MIC_DIAGNOSTICS
    events=nullptr;
    frames=errors=overflows=clipped=peak=0;absoluteSum=0;
#endif
}
