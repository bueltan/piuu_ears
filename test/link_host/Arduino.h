#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <vector>
#include <mutex>
#include <string>
#include <cstdio>
#include "freertos/FreeRTOS.h"
uint32_t millis();
static constexpr int SERIAL_8N1 = 0;
struct SerialData {
    std::deque<uint8_t> rx;
    std::vector<uint8_t> tx;
    std::mutex mutex;
};
extern SerialData serialData[2];
struct Logger {
    std::vector<std::string> lines;
    template<class... Args> void printf(const char *format, Args... args) {
        char buffer[1024];std::snprintf(buffer,sizeof(buffer),format,args...);lines.emplace_back(buffer);
    }
    void println(const char *) {}
};
struct MockEsp { unsigned getFreeHeap() const { return 100000; } };
static MockEsp ESP;
// The cross-project test exercises TX only; never start the infinite RX task.
using UBaseType_t = unsigned;
using TickType_t = unsigned;
using QueueHandle_t = void *;
using TaskHandle_t = void *;
static constexpr int pdPASS = 1;
inline TickType_t pdMS_TO_TICKS(unsigned n) { return n; }
inline void vTaskDelay(TickType_t) {}
inline QueueHandle_t xQueueCreate(unsigned, size_t) { return &ESP; }
inline void vQueueDelete(QueueHandle_t) {}
inline int xQueueSend(QueueHandle_t, const void *, TickType_t) { return pdTRUE; }
inline int xQueueReceive(QueueHandle_t, void *, TickType_t) { return 0; }
inline unsigned uxQueueMessagesWaiting(QueueHandle_t) { return 0; }
inline int xTaskCreate(void (*)(void *), const char *, unsigned, void *, unsigned, TaskHandle_t *) { return pdPASS; }
extern Logger Serial;
class HardwareSerial {
    int id;
public:
    explicit HardwareSerial(int n): id(n) {}
    void begin(uint32_t, int, int, int) {}
    void setRxBufferSize(size_t) {}
    void flush() {}
    void end() {serialData[id].rx.clear();}
    size_t write(const uint8_t *p, size_t n) {
        std::lock_guard<std::mutex> guard(serialData[id].mutex);
        serialData[id].tx.insert(serialData[id].tx.end(),p,p+n);
        return n;
    }
    int available() { return serialData[id].rx.size(); }
    size_t read(uint8_t *p, size_t n) {
        size_t count = 0;
        while (count < n && available()) p[count++] = uint8_t(read());
        return count;
    }
    int read() {
        if (serialData[id].rx.empty()) return -1;
        const auto b=serialData[id].rx.front(); serialData[id].rx.pop_front(); return b;
    }
};
