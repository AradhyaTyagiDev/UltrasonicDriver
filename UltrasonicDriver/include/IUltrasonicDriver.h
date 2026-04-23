#pragma once

#include <freertos/FreeRTOS.h>
#include "UltrasonicTypes.h"

class IUltrasonicDriver
{
public:
    virtual ~IUltrasonicDriver() = default;

    virtual void begin() = 0;
    virtual void startReceive(UltrasonicSensorId sensor) = 0;

#ifdef UNIT_TEST
    virtual void simulate(UltrasonicSensorId sensor, uint32_t duration) = 0;
#endif
};