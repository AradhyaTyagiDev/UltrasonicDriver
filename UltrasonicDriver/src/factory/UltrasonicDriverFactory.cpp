
#include "UltrasonicDriverFactory.h"

// ================= DRIVER INCLUDES =================

// 🔥 Order matters: ARDUINO first
#if defined(ULTRASONIC_USE_MOCK)

#include "drivers/mock/UltrasonicMockDriver.h"

#elif defined(ARDUINO)

#include "drivers/arduino/UltrasonicArduinoISRDriver.h"

#elif defined(ESP_PLATFORM)

#include "drivers/esp_idf/UltrasonicRMTDriver.h"

#else
#error "No supported platform for UltrasonicDriver"
#endif

// ================= FACTORY =================
std::unique_ptr<IUltrasonicDriver> createUltrasonicDriver(
    const std::vector<UltrasonicConfig> &configs,
    QueueHandle_t queue)
{
#if defined(ULTRASONIC_USE_MOCK)

    return std::unique_ptr<IUltrasonicDriver>(
        new UltrasonicMockDriver(configs, queue));

#elif defined(ARDUINO)

    // Arduino framework driver keeps the same queue/event contract as RMT.
    return std::unique_ptr<IUltrasonicDriver>(
        new UltrasonicArduinoISRDriver(configs, queue));

#elif defined(ESP_PLATFORM)
    // ESP-IDF RMT driver
    return std::unique_ptr<IUltrasonicDriver>(
        new UltrasonicRMTDriver(configs, queue));

#else
#error "No ultrasonic driver selected"
#endif
}

UltrasonicDriverContext createUltrasonicDriverContext(
    const std::vector<UltrasonicConfig> &configs,
    QueueHandle_t queue)
{
    UltrasonicDriverContext ctx;

    ctx.driver = createUltrasonicDriver(configs, queue);
    ctx.driver->begin();

    ctx.test = ctx.driver->testHook();

    return ctx;
}
