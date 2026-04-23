#pragma once
#include <memory>
#include <vector>
#include "IUltrasonicDriver.h"
#include "UltrasonicTypes.h"

#if defined(ESP_PLATFORM)
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#endif

// Factory API
std::unique_ptr<IUltrasonicDriver> createUltrasonicDriver(
    const std::vector<UltrasonicConfig> &configs,
#if defined(ESP_PLATFORM)
    QueueHandle_t queue
#else
    void *queue = nullptr // fallback for non-RTOS platforms
#endif
);