#pragma once
#include <stdint.h>

#include "UltrasonicSensorTypes.h"

enum class UltrasonicSensorId : uint8_t
{
    FRONT,
    FRONT_LEFT,
    LEFT,
    FRONT_RIGHT,
    RIGHT,
    REAR_LEFT,
    REAR,
    REAR_RIGHT,
    COUNT
};

struct SensorGeometry
{
    float dirX; // normalized direction vector
    float dirY;
};

struct MotionVector
{
    float x; // direction
    float y;
    float speed; // magnitude
};

struct UltrasonicConfig
{
    uint8_t trigPin;
    uint8_t echoPin;
    // Stagger intervals to avoid ultrasonic interference: Sensor Interference Avoided: 50ms, 70ms, 90ms, 110ms
    uint32_t minIntervalMs;

    float basePriority; // base weight (e.g., 1.0–10.0)
    SensorGeometry geometry;

    char label; // 'F', 'L', 'R', 'B'
};