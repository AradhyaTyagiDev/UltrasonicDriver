#pragma once

#include <stdint.h>
#include "UltrasonicSensorTypes.h"

/// @brief message container used to transfer data from: ISR (RMT interrupt) ➜ Main application (UltrasonicManager)
/// Step 1: RMT captures echo pulse
/// Step 2: ISR fires EchoEvent: xQueueSendFromISR(queueHandle, &evt, NULL); -> Pack data -> Push to queue -> Exit FAST ⚡
/// Step 3: Main loop processes it asyncronously
/// ISR = data producer
/// Manager = data consumer
/// Enables Scalability (1 → 8 sensors): Same queue handles all sensors: Queue = thread-safe communication
struct UltrasonicEchoEvent
{
    UltrasonicSensorId sensorId;
    uint32_t duration;  // µs : distance = duration * 0.0343 / 2;
    uint32_t timestamp; // micros()
    bool timeout;
};