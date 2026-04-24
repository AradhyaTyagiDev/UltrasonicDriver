
#include "UltrasonicDriverFactory.h"

// ================= DRIVER INCLUDES =================

// 🔥 Order matters: ARDUINO first
#if defined(ULTRASONIC_USE_MOCK)

#include "drivers/mock/UltrasonicMockDriver.h"

#elif defined(ARDUINO)

#include "drivers/arduino/UltrasonicArduinoDriver.h"

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

    return std::make_unique<UltrasonicMockDriver>(configs, queue);

#elif defined(ARDUINO)

    // Arduino driver ignores queue internally
    return std::make_unique<UltrasonicArduinoDriver>(configs);

#elif defined(ESP_PLATFORM)
    // ESP-IDF RMT driver
    return std::make_unique<UltrasonicRMTDriver>(configs, queue);

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