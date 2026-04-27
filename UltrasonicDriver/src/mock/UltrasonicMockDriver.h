#pragma once

#include <vector>
#include <queue>
#include <stdint.h>

#include "IUltrasonicDriver.h"
#include "IUltrasonicEventReceiver.h"
#include "UltrasonicDriverTypes.h"

class UltrasonicMockDriver : public IUltrasonicDriver, public IUltrasonicDriverTestHook
{
public:
    UltrasonicMockDriver(const std::vector<UltrasonicConfig> &cfg,
                         IUltrasonicEventReceiver &receiver);

    IUltrasonicDriverTestHook *testHook() override
    {
        return this;
    }

    void begin() override;
    void startReceive(UltrasonicSensorId sensor) override;

    // Stats
    uint32_t getTotalDrops() const override;
    uint32_t getSensorDrops(UltrasonicSensorId sensor) const override;
    uint32_t getErrorCount() const override;

    // 🔥 Manual injection (instant)
    void simulate(UltrasonicSensorId sensor, uint32_t duration);

    // 🔥 Scheduled event (delayed response)
    void schedule(UltrasonicSensorId sensor, uint32_t duration, uint32_t delayMs);

    // 🔥 Timeout simulation
    void simulateTimeout(UltrasonicSensorId sensor);

    // 🔥 Advance time manually (for deterministic tests)
    void tick(uint32_t deltaMs);

private:
    struct ScheduledEvent
    {
        UltrasonicSensorId sensor;
        uint32_t duration;
        uint32_t triggerTime;
        bool timeout;
    };

    std::vector<UltrasonicConfig> configs;
    IUltrasonicEventReceiver &eventReceiver;

    uint32_t currentTimeMs = 0;

    // eventQueues = delayed ISR simulator
    // Each ultrasonic sensor behaves independently: Sensor 0 → echo at t=10ms
    // Sensor 1 → echo at t=15ms (no collision)
    // Sensor 2 → timeout at t=50ms
    // Each sensor gets its own event queue (no collision, no dynamic alloc)
    std::vector<std::queue<ScheduledEvent>> eventQueues;

    // Stats
    std::vector<uint32_t> dropCounter;
    uint32_t totalDrops = 0;
    uint32_t errorCount = 0;

    void pushEvent(const ScheduledEvent &evt);
};
