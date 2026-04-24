#pragma once
#include <memory>
#include <vector>
#include "IUltrasonicDriver.h"
#include "UltrasonicTypes.h"
#include "UltrasonicDriverContext.h"
#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#endif

// Factory API
UltrasonicDriverContext createUltrasonicDriverContext(
    const std::vector<UltrasonicConfig> &configs,
#if defined(ULTRASONIC_USE_MOCK)
    QueueHandle_t queue
#elif defined(ESP_PLATFORM)
    QueueHandle_t queue
#else
    void *queue = nullptr // fallback for non-RTOS platforms
#endif
);