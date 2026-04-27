#include "UltrasonicArduinoISRDriver.h"

#include <assert.h>
#include "driver/UltrasonicUtils.h"

UltrasonicArduinoISRDriver::UltrasonicArduinoISRDriver(
    const std::vector<UltrasonicConfig> &cfg,
    IUltrasonicEventReceiver &receiver)
    : configs(cfg), eventReceiver(receiver)
{
    assert(!configs.empty() && configs.size() <= kMaxSensors);

    states.resize(configs.size());
    isrContexts.resize(configs.size());
    dropCounter.resize(configs.size(), 0);
}

// ============================================================
// INIT
// ============================================================

void UltrasonicArduinoISRDriver::begin()
{
    for (size_t i = 0; i < configs.size(); ++i)
    {
        pinMode(configs[i].echoPin, INPUT);

        isrContexts[i].driver = this;
        isrContexts[i].index = static_cast<uint8_t>(i);

        attachEchoInterrupt(i);
    }
}

// ============================================================
// START RECEIVE
// ============================================================

void UltrasonicArduinoISRDriver::startReceive(UltrasonicSensorId sensor)
{
    const size_t idx = toIndex(sensor);

    if (idx >= states.size())
    {
        __atomic_add_fetch(&errorCounter, 1, __ATOMIC_RELAXED);
        return;
    }

    noInterrupts();
    states[idx].risingEdgeUs = 0;
    states[idx].highSeen = false;
    states[idx].armed = true;
    interrupts();
}

// ============================================================
// STATS
// ============================================================

uint32_t UltrasonicArduinoISRDriver::getTotalDrops() const
{
    return __atomic_load_n(&totalDrops, __ATOMIC_RELAXED);
}

uint32_t UltrasonicArduinoISRDriver::getSensorDrops(UltrasonicSensorId sensor) const
{
    const size_t idx = toIndex(sensor);

    if (idx >= dropCounter.size())
        return 0;

    return __atomic_load_n(&dropCounter[idx], __ATOMIC_RELAXED);
}

uint32_t UltrasonicArduinoISRDriver::getErrorCount() const
{
    return __atomic_load_n(&errorCounter, __ATOMIC_RELAXED);
}

// ============================================================
// ISR SETUP
// ============================================================

void UltrasonicArduinoISRDriver::attachEchoInterrupt(size_t idx)
{
    attachInterruptArg(configs[idx].echoPin,
                       UltrasonicArduinoISRDriver::handleInterrupt,
                       &isrContexts[idx],
                       CHANGE);
}

// ============================================================
// ISR ENTRY
// ============================================================

void IRAM_ATTR UltrasonicArduinoISRDriver::handleInterrupt(void *arg)
{
    auto *ctx = static_cast<IsrContext *>(arg);

    if (ctx && ctx->driver)
        ctx->driver->onEdge(ctx->index);
}

// ============================================================
// EDGE HANDLER
// ============================================================

void IRAM_ATTR UltrasonicArduinoISRDriver::onEdge(uint8_t idx)
{
    if (idx >= states.size())
        return;

    auto &state = states[idx];

    if (!state.armed)
        return;

    const uint32_t now = micros(); // acceptable for now
    const bool levelHigh = digitalRead(configs[idx].echoPin);

    if (levelHigh)
    {
        state.risingEdgeUs = now;
        state.highSeen = true;
        return;
    }

    if (!state.highSeen)
        return;

    const uint32_t duration = now - state.risingEdgeUs;

    state.armed = false;
    state.highSeen = false;

    if (duration == 0 || duration > kMaxEchoDurationUs)
    {
        pushEventFromISR(idx, 0, true);
        return;
    }

    pushEventFromISR(idx, duration, false);
}

// ============================================================
// PUSH EVENT (ISR SAFE)
// ============================================================

void IRAM_ATTR UltrasonicArduinoISRDriver::pushEventFromISR(
    uint8_t idx,
    uint32_t duration,
    bool timeout)
{
    UltrasonicEchoEvent evt{};
    evt.sensorId = static_cast<UltrasonicSensorId>(idx);
    evt.duration = duration;
    evt.timestamp = micros(); // optional: can be removed for strict ISR safety
    evt.timeout = timeout;

    // 🚫 NO FreeRTOS concept here anymore
    if (!eventReceiver.pushFromISR(evt))
    {
        recordDropFromISR(idx);
    }
}

// ============================================================
// DROP TRACKING
// ============================================================

void IRAM_ATTR UltrasonicArduinoISRDriver::recordDropFromISR(uint8_t idx)
{
    if (idx < dropCounter.size())
        __atomic_add_fetch(&dropCounter[idx], 1, __ATOMIC_RELAXED);

    __atomic_add_fetch(&totalDrops, 1, __ATOMIC_RELAXED);
}