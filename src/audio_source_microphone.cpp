#include "audio_source_microphone.h"

#include "config.h"
#include "microphone_pcm.h"
#include <driver/i2s.h>
#include <esp_intr_alloc.h>
#include <string.h>

namespace {
constexpr i2s_port_t kMicPort = I2S_NUM_0;
constexpr uint32_t kReadTimeoutMs = 100;
int32_t rawFrame[FRAME_SAMPLES];
bool initialized = false;
#if EARS_MIC_DIAGNOSTICS
QueueHandle_t events = nullptr;
uint32_t frames = 0, errors = 0, overflows = 0, clipped = 0;
uint32_t peak = 0, lastReport = 0;
uint64_t absoluteSum = 0;

void reportDiagnostics()
{
    i2s_event_t event;
    while (events && xQueueReceive(events, &event, 0) == pdTRUE) {
        if (event.type == I2S_EVENT_RX_Q_OVF) ++overflows;
    }
    const uint32_t now = millis();
    if (now - lastReport < 5000) return;
    Serial.printf("[MIC] frames=%lu peak=%lu mean_abs=%lu clipped=%lu read_errors=%lu rx_overflow=%lu\n",
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
        Serial.printf("[MIC] driver_install failed: %s\n", esp_err_to_name(err));
        return false;
    }
    err = i2s_set_pin(kMicPort, &pins);
    if (err != ESP_OK) {
        Serial.printf("[MIC] set_pin failed: %s\n", esp_err_to_name(err));
        i2s_driver_uninstall(kMicPort);
        return false;
    }
    initialized = true;
#if EARS_MIC_DIAGNOSTICS
    lastReport = millis();
#endif
    Serial.printf("[MIC] INMP441 I2S0 BCLK=%d WS=%d SD=%d, %lu Hz, 32-bit RIGHT -> PCM16, shift=%u\n",
        MIC_BCLK_PIN, MIC_WS_PIN, MIC_DATA_PIN,
        static_cast<unsigned long>(SAMPLE_RATE_HZ), MIC_PCM_SHIFT);
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
    for (size_t i = 0; i < FRAME_SAMPLES; ++i) {
        const int16_t sample = microphonePcm16(rawFrame[i], MIC_PCM_SHIFT);
        // Explicit little-endian output; do not assume byte-buffer alignment.
        const uint16_t bits = static_cast<uint16_t>(sample);
        buffer[i * 2] = static_cast<uint8_t>(bits);
        buffer[i * 2 + 1] = static_cast<uint8_t>(bits >> 8);
#if EARS_MIC_DIAGNOSTICS
        const uint32_t magnitude = sample < 0 ? -int32_t(sample) : sample;
        if (magnitude > peak) peak = magnitude;
        absoluteSum += magnitude;
        if (sample == INT16_MIN || sample == INT16_MAX) ++clipped;
#endif
    }
#if EARS_MIC_DIAGNOSTICS
    ++frames;
    reportDiagnostics();
#endif
    // I2S DMA paces live capture. No additional 20 ms task delay is needed.
    return true;
}
