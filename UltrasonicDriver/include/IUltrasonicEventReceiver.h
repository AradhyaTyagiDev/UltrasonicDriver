#pragma once

#include "UltrasonicTypes.h"

// OS independence; ISR safety abstraction
class IUltrasonicEventReceiver
{
public:
    virtual ~IUltrasonicEventReceiver() = default;

    // Normal context (task / loop) // May be blocking, can allocate memory, can log
    virtual bool push(const UltrasonicEchoEvent &event) = 0;

    // ISR-safe push (must be non-blocking) be fast
    // allocate memory ❌, log ❌, block ❌
    virtual bool pushFromISR(const UltrasonicEchoEvent &event) = 0;
};