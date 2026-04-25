#pragma once

#include <stdint.h>
#include <vector>

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "IUltrasonicDriver.h"
#include "UltrasonicTypes.h"

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

class UltrasonicArduinoISRDriver : public IUltrasonicDriver
{
public:
    UltrasonicArduinoISRDriver(const std::vector<UltrasonicConfig> &cfg,
                               QueueHandle_t queue);

    void begin() override;
    void startReceive(UltrasonicSensorId sensor) override;

    uint32_t getTotalDrops() const override;
    uint32_t getSensorDrops(UltrasonicSensorId sensor) const override;
    uint32_t getErrorCount() const override;

private:
    static constexpr size_t kMaxSensors = 4;
    static constexpr uint32_t kMaxEchoDurationUs = 30000;

    struct SensorState
    {
        volatile uint32_t risingEdgeUs = 0;
        volatile bool armed = false;
        volatile bool highSeen = false;
    };

    struct IsrContext
    {
        UltrasonicArduinoISRDriver *driver = nullptr;
        uint8_t index = 0;
    };

    std::vector<UltrasonicConfig> configs;
    QueueHandle_t echoQueue = nullptr;
    std::vector<SensorState> states;
    std::vector<IsrContext> isrContexts;
    std::vector<uint32_t> dropCounter;

    uint32_t totalDrops = 0;
    uint32_t errorCounter = 0;

    void attachEchoInterrupt(size_t idx);
    void IRAM_ATTR onEdge(uint8_t idx);
    void IRAM_ATTR pushEventFromISR(uint8_t idx, uint32_t duration, bool timeout);
    void IRAM_ATTR recordDropFromISR(uint8_t idx);

    static void IRAM_ATTR handleInterrupt(void *arg);
};
