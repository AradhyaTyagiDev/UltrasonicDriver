#pragma once

#include "freertos/FreeRTOS.h"

/// @brief define Inside UltrasonicManager or Pass as dependency (clean architecture): ❌ Global variables → hard to control
struct UltrasonicTuning
{
    // ================================
    // 🔷 Scheduling / Timing
    // ================================
    struct Timing
    {
        uint32_t interSensorGapUs = 3000; // gap between triggers (3–5ms)
    } timing;

    // ================================
    // 🔷 Relevance / Selection
    // ================================
    struct Relevance
    {
        float threshold = 5.0f; // minimum score to consider sensor
    } relevance;

    // ================================
    // 🔷 Scoring Weights
    // ================================
    struct Scoring
    {
        float basePriority = 1.0f;     // default baseline (can be overridden per config)
        float alignmentWeight = 20.0f; // motion-direction importance
        float riskWeight = 100.0f;     // distance-based risk

        float closeDistanceThreshold = 30.0f; // cm
        float closeDistanceBoost = 50.0f;     // strong boost when very close

        float speedScale = 1.0f; // how speed affects score

        float velocityWeight = 40.0f;  // predictive boost
        float confidenceWeight = 1.0f; // scale final score
    } scoring;

    // ================================
    // 🔷 Noise Model
    // ================================
    struct Noise
    {
        // Valid measurement range
        float minValidDistance = 2.0f;   // cm (sensor limitation)
        float maxValidDistance = 400.0f; // cm

        // Soft zones (gradual degradation)
        float nearMinZoneWidth = 5.0f;  // cm from min
        float nearMaxZoneWidth = 50.0f; // cm from max

        // Penalty strengths (0 → 1)
        float nearMinPenalty = 0.3f;
        float nearMaxPenalty = 0.3f;

        // Timeout = worst case
        float timeoutNoise = 1.0f;

        // Threshold to classify as noisy (for boolean use)
        float threshold = 0.7f;
    } noise;

    struct GroupBias
    {
        float frontBoost = 20.0f;
        float sideBoost = 5.0f;
        float rearPenalty = -10.0f;
    } groupBias;

    struct Scheduling
    {
        float hysteresisBoost = 5.0f;
    } scheduling;
};

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