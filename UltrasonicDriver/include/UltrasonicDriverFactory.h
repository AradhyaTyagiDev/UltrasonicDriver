#pragma once
#include <memory>
#include <vector>
#include "IUltrasonicDriver.h"
#include "UltrasonicDriverTypes.h"
#include "UltrasonicDriverContext.h"

/**
 * Creates a fully initialized ultrasonic system.
 *
 * @param configs   Sensor configurations
 * @param nativeHandle
 *        Optional platform-specific handle.
 *
 *        Interpretation depends on platform:
 *        - ESP32 (FreeRTOS): QueueHandle_t
 *        - Arduino: nullptr (internal ring buffer)
 *        - STM32: buffer/config pointer
 *        - Linux: queue/context pointer
 *
 * @return UltrasonicDriverContext
 */
UltrasonicDriverContext createUltrasonicDriverContext(
    const std::vector<UltrasonicConfig> &configs,
    void *queue = nullptr);