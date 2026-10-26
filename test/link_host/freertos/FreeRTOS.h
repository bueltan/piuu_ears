#pragma once
#include <mutex>
using SemaphoreHandle_t = std::mutex *;
static constexpr int pdTRUE = 1;
static constexpr int portMAX_DELAY = -1;

using portMUX_TYPE=std::mutex;
#define portMUX_INITIALIZER_UNLOCKED {}
#define portENTER_CRITICAL(m) (m)->lock()
#define portEXIT_CRITICAL(m) (m)->unlock()
