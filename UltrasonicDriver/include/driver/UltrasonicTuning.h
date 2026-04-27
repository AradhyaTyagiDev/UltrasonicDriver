#pragma once

#include <stdint.h>
#include "UltrasonicSensorTypes.h"

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