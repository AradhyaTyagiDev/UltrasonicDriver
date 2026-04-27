#pragma once

#include <UltrasonicSensorTypes.h>
#include "driver/UltrasonicDriverFactory.h"

#include <memory>
#include <utility>

// ================= PLATFORM RECEIVERS =================

#if defined(ULTRASONIC_DRIVER_ESP_IDF_RMT) || defined(ULTRASONIC_DRIVER_ESP32_ARDUINO_ISR)
#include "driver/FreeRTOSEventReceiver.h"
#endif

#if defined(ULTRASONIC_DRIVER_ARDUINO)
#include "RingBufferEventReceiver.h"
#endif

// ================= DRIVER INCLUDES =================

#if defined(ULTRASONIC_USE_MOCK)

#include "mock/UltrasonicMockDriver.h"

#elif defined(ULTRASONIC_DRIVER_ESP32_ARDUINO_ISR)

#include "drivers/esp32_arduino_isr/UltrasonicArduinoISRDriver.h"

#elif defined(ULTRASONIC_DRIVER_ESP_IDF_RMT)

#include "drivers/esp_idf_rmt/UltrasonicRMTDriver.h"

#else
#error "No supported platform for UltrasonicDriver"
#endif

// ============================================================
// INTERNAL DRIVER FACTORY
// ============================================================

static std::unique_ptr<IUltrasonicDriver> createDriver(
    const std::vector<UltrasonicConfig> &configs,
    IUltrasonicEventReceiver &receiver)
{
#if defined(ULTRASONIC_USE_MOCK)

    return std::make_unique<UltrasonicMockDriver>(configs, receiver);

#elif defined(ULTRASONIC_DRIVER_ESP32_ARDUINO_ISR)

    return std::unique_ptr<IUltrasonicDriver>(new UltrasonicArduinoISRDriver(configs, receiver));

#elif defined(ULTRASONIC_DRIVER_ESP_IDF_RMT)

    return std::make_unique<UltrasonicRMTDriver>(configs, receiver);

#else
#error "No ultrasonic driver selected"
#endif
}

// ============================================================
// MAIN FACTORY (PUBLIC API)
// ============================================================
UltrasonicDriverContext createUltrasonicDriverContext(
    const std::vector<UltrasonicConfig> &configs,
    void *nativeHandle)
{
    UltrasonicDriverContext ctx;

    // ================= CREATE RECEIVER =================

#if defined(ULTRASONIC_DRIVER_ESP_IDF_RMT) || defined(ULTRASONIC_DRIVER_ESP32_ARDUINO_ISR)

    // Expect FreeRTOS queue
    auto queue = static_cast<QueueHandle_t>(nativeHandle);

    // Basic safety (optional but recommended)
    if (queue == nullptr)
    {
        // You can assert or fallback
        // For now: fail-fast
        abort();
    }
    ctx.receiver = std::unique_ptr<IUltrasonicEventReceiver>(new FreeRTOSEventReceiver(queue));

#elif defined(ULTRASONIC_DRIVER_ARDUINO)

    // No external handle needed
    ctx.receiver = std::make_unique<RingBufferEventReceiver>();

#else
#error "No receiver implementation for this platform"
#endif

    // ================= CREATE DRIVER =================

    ctx.driver = createDriver(configs, *ctx.receiver);

    // ================= INIT =================

    ctx.driver->begin();

    // ================= TEST HOOK =================

    ctx.test = ctx.driver->testHook();

    return ctx;
}