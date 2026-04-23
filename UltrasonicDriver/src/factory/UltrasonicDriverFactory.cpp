
#include "UltrasonicDriverFactory.h"

// ================= DRIVER INCLUDES =================

// 🔥 Order matters: ARDUINO first
#if defined(ARDUINO)

#include "drivers/arduino/UltrasonicArduinoDriver.h"

#elif defined(ESP_PLATFORM)

#include "drivers/esp_idf/UltrasonicRMTDriver.h"

#elif defined(ULTRASONIC_USE_MOCK)

#include "drivers/mock/UltrasonicMockDriver.h"

#else
#error "No supported platform for UltrasonicDriver"
#endif

// ================= FACTORY =================

std::unique_ptr<IUltrasonicDriver> createUltrasonicDriver(
    const std::vector<UltrasonicConfig> &configs,
#if defined(ESP_PLATFORM)
    QueueHandle_t queue
#else
    void *queue
#endif
)
{
#if defined(ARDUINO)

    // Arduino driver (no RTOS queue needed)
    return std::make_unique<UltrasonicArduinoDriver>(configs);

#elif defined(ESP_PLATFORM)

    // ESP-IDF RMT driver
    return std::make_unique<UltrasonicRMTDriver>(configs, queue);

#elif defined(ULTRASONIC_USE_MOCK)

    // Mock driver for testing
    return std::make_unique<UltrasonicMockDriver>(configs);

#else
    static_assert(false, "No ultrasonic driver selected");
#endif
}