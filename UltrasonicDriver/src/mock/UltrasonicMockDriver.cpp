#include "UltrasonicMockDriver.h"

#include <assert.h>
#include <UltrasonicUtils.h>

// =============================
// Constructor
// =============================
UltrasonicMockDriver::UltrasonicMockDriver(
    const std::vector<UltrasonicConfig> &cfg,
    QueueHandle_t queue)
    : configs(cfg), echoQueue(queue)
{
    assert(queue != nullptr);

    eventQueues.resize(configs.size());
    dropCounter.resize(configs.size(), 0);
}

// =============================
// Begin
// =============================
void UltrasonicMockDriver::begin()
{
    // nothing needed
}

// =============================
// Start Receive
// =============================
void UltrasonicMockDriver::startReceive(UltrasonicSensorId sensor)
{
    const size_t idx = toIndex(sensor);

    if (idx >= eventQueues.size())
        return;

    auto &q = eventQueues[idx];

    if (q.empty())
    {
        // simulate timeout automatically
        ScheduledEvent evt{};
        evt.sensor = sensor;
        evt.timeout = true;
        evt.triggerTime = currentTimeMs + 50; // default timeout

        q.push(evt);
    }
}

// =============Stats================
uint32_t UltrasonicMockDriver::getTotalDrops() const
{
    return totalDrops;
}

uint32_t UltrasonicMockDriver::getSensorDrops(UltrasonicSensorId sensor) const
{
    size_t idx = static_cast<size_t>(sensor);

    if (idx >= dropCounter.size())
        return 0;

    return dropCounter[idx];
}

uint32_t UltrasonicMockDriver::getErrorCount() const
{
    return errorCount;
}

// =============================
// Manual Instant Simulation
// =============================
void UltrasonicMockDriver::simulate(UltrasonicSensorId sensor, uint32_t duration)
{
    const size_t idx = toIndex(sensor);

    if (idx >= eventQueues.size())
        return;

    ScheduledEvent evt{};
    evt.sensor = sensor;
    evt.duration = duration;
    evt.triggerTime = currentTimeMs;
    evt.timeout = false;

    eventQueues[idx].push(evt);
}

// =============================
// Delayed Simulation
// =============================
void UltrasonicMockDriver::schedule(UltrasonicSensorId sensor,
                                    uint32_t duration,
                                    uint32_t delayMs)
{
    const size_t idx = toIndex(sensor);

    if (idx >= eventQueues.size())
        return;

    ScheduledEvent evt{};
    evt.sensor = sensor;
    evt.duration = duration;
    evt.triggerTime = currentTimeMs + delayMs;
    evt.timeout = false;

    eventQueues[idx].push(evt);
}

// =============================
// Timeout Simulation
// =============================
void UltrasonicMockDriver::simulateTimeout(UltrasonicSensorId sensor)
{
    const size_t idx = toIndex(sensor);

    if (idx >= eventQueues.size())
        return;

    ScheduledEvent evt{};
    evt.sensor = sensor;
    evt.duration = 0;
    evt.timeout = true;
    evt.triggerTime = currentTimeMs + 50;

    eventQueues[idx].push(evt);
}

// =============Time Advancement Engine================
// When caller call: driver.schedule(0, 1200, 10); -> eventQueues stores event for sensor 0 at t=10ms but 👉 Nothing happens immediately.
// When you call: driver.tick(20); then real event is pushed to queue at t=10ms (since 20ms > 10ms) and can be received by manager/task.
// If you call tick(5) → event is not pushed since currentTime = 5ms < 10ms.
void UltrasonicMockDriver::tick(uint32_t deltaMs)
{
    currentTimeMs += deltaMs;

    for (size_t i = 0; i < eventQueues.size(); i++)
    {
        auto &q = eventQueues[i];

        while (!q.empty())
        {
            const auto &evt = q.front();

            if (evt.triggerTime > currentTimeMs)
                break;

            pushEvent(evt);
            q.pop();
        }
    }
}

// =============Push to Queue (simulate ISR)================
void UltrasonicMockDriver::pushEvent(const ScheduledEvent &e)
{
    UltrasonicEchoEvent evt{};
    evt.sensorId = static_cast<UltrasonicSensorId>(e.sensor);
    evt.duration = e.duration;
    evt.timestamp = currentTimeMs;
    evt.timeout = e.timeout;

    if (xQueueSend(echoQueue, &evt, 0) != pdPASS)
    {
        size_t idx = toIndex(e.sensor);

        dropCounter[idx]++;
        totalDrops++;
    }
}