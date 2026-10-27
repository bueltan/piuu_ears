#pragma once
#include <cstdint>
#include <cstddef>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdio>
using BaseType_t=int;
constexpr int pdPASS=1;
static std::thread worker;
static std::atomic<bool> finish{false};
struct EndTask {};
inline uint32_t millis(){static const auto start=std::chrono::steady_clock::now();return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();}
inline void delay(unsigned ms){std::this_thread::sleep_for(std::chrono::milliseconds(ms));}
inline unsigned pdMS_TO_TICKS(unsigned ms){return ms;}
inline void vTaskDelay(unsigned ms){if(finish.load())throw EndTask{};delay(ms);}
inline int xTaskCreate(void(*fn)(void*),const char*,unsigned,void*,unsigned,void*){worker=std::thread([fn]{try{fn(nullptr);}catch(EndTask&) {}});return pdPASS;}
struct Logger {void println(const char*){}template<typename... A> void printf(const char*,A...){} };
static Logger Serial;
