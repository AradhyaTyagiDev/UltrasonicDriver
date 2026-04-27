#pragma once

#include <stdint.h>
#include <cstdint>
#include <vector>
#include <queue>

#include "UltrasonicSensorTypes.h"

// UltrasonicState state[N];     // final output
struct UltrasonicState
{
    uint32_t lastRun = 0;

    float distance = -1.0f;
    /// final published output
    float confidence = 0.0f;

    float dynamicPriority = 0.0f;

    bool valid = false;
};

enum class SensorGroup : uint8_t
{
    FRONT,
    SIDE,
    REAR
};

inline SensorGroup getSensorGroup(UltrasonicSensorId id)
{
    switch (id)
    {
    case UltrasonicSensorId::FRONT:
    case UltrasonicSensorId::FRONT_LEFT:
    case UltrasonicSensorId::FRONT_RIGHT:
        return SensorGroup::FRONT;

    case UltrasonicSensorId::LEFT:
    case UltrasonicSensorId::RIGHT:
        return SensorGroup::SIDE;

    case UltrasonicSensorId::REAR:
    case UltrasonicSensorId::REAR_LEFT:
    case UltrasonicSensorId::REAR_RIGHT:
        return SensorGroup::REAR;

    default:
        return SensorGroup::SIDE;
    }
}

enum class RobotMotionState : uint8_t
{
    FORWARD,
    TURN_LEFT,
    TURN_RIGHT,
    BACKWARD,
    IDLE
};

/// State Machine
enum class UltrasonicSensorPhase : uint8_t
{
    IDLE,
    TRIGGERED,
    WAITING,
    RECEIVED,
    PROCESSING,
    DONE
};

/// @brief Runtime Behavior: live state container of one sensor
/// 1. When sensor was triggered
/// 2. Whether echo started/ended
/// 3. Whether result is ready
/// 4. Timeout handling
/// 5. Current state in pipeline
struct UltrasonicRuntime
{
    UltrasonicSensorPhase phase = UltrasonicSensorPhase::IDLE;
    // Timestamp when TRIG pulse was sent
    uint32_t triggerTime = 0;
    // echoStart = rising edge time
    // echoEnd = falling edge time. duration = echoEnd - echoStart
    uint32_t duration = 0; // from RMT directly
    // Updated when echo received
    float distance = -1.0f;
    float previousDistance = -1.0f;

    float relativeVelocity = 0.0f; // cm/s
    uint32_t lastUpdateTime = 0;   // for dt calculation

    // Flag set by ISR/queue when echo arrives
    bool echoReceived = false;
    bool timeout = false;

    bool enabled = true;
    float dynamicPriority = 0.0f;
    /// internal computation
    float confidence = 1.0f; // 0 → 1
};