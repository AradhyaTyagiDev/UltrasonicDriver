#pragma once

#include "IUltrasonicEventReceiver.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Adapter Implementation: FreeRTOS Queue as event receiver. Driver is portable. RTOS independent
// Hide RTOS detail inside adapter
// Driver (ISR) → Interface → Platform Adapter → OS Primitive
/*
Ultrasonic Driver
        ↓
IUltrasonicEventReceiver
        ↓
FreeRTOSEventReceiver (FOR ESP32)
        ↓
FreeRTOS Queue
 */
class FreeRTOSEventReceiver : public IUltrasonicEventReceiver
{
public:
    explicit FreeRTOSEventReceiver(QueueHandle_t queue);

    bool push(const UltrasonicEchoEvent &event) override;
    bool pushFromISR(const UltrasonicEchoEvent &event) override;

private:
    QueueHandle_t queue = nullptr;
};