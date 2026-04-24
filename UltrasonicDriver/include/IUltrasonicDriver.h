#pragma once

#include <freertos/FreeRTOS.h>
#include "UltrasonicTypes.h"

// FIX: forward declaration
class IUltrasonicDriverTestHook;

class IUltrasonicDriver
{
public:
    virtual ~IUltrasonicDriver() = default;

    virtual void begin() = 0;
    virtual void startReceive(UltrasonicSensorId sensor) = 0;

    // Stats
    virtual uint32_t getTotalDrops() const = 0;
    virtual uint32_t getSensorDrops(UltrasonicSensorId sensor) const = 0;
    virtual uint32_t getErrorCount() const = 0;

    // Composition-friendly test hook (optional)
    virtual IUltrasonicDriverTestHook *testHook() { return nullptr; }
};

class IUltrasonicDriverTestHook
{
public:
    virtual ~IUltrasonicDriverTestHook() = default;
    // 🔥 Manual injection (instant)
    virtual void simulate(UltrasonicSensorId sensor, uint32_t duration) = 0;

    // 🔥 Scheduled event (delayed response)
    virtual void schedule(UltrasonicSensorId sensor, uint32_t duration, uint32_t delayMs) = 0;

    // 🔥 Timeout simulation
    virtual void simulateTimeout(UltrasonicSensorId sensor) = 0;

    // 🔥 Advance time manually (for deterministic tests)
    virtual void tick(uint32_t deltaMs) = 0;
};
