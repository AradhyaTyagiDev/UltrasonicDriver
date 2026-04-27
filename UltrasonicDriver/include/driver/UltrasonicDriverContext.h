#pragma once

#include <memory>

#include "IUltrasonicDriver.h"
#include "IUltrasonicEventReceiver.h"

// Unified runtime context for ultrasonic system
// Owns driver + receiver and exposes optional test hooks safely
// 🔥 Clean ownership + access separation
// Provide a unified context for driver, event receiver, and test hooks. This simplifies dependency management and testability.
struct UltrasonicDriverContext
{
    // Ownership: managed here
    std::unique_ptr<IUltrasonicDriver> driver;
    std::unique_ptr<IUltrasonicEventReceiver> receiver;

    // Non-owning test interface (optional)
    IUltrasonicDriverTestHook *test = nullptr;

    // ---- Safety helpers ----

    bool isValid() const
    {
        return driver != nullptr && receiver != nullptr;
    }

    bool supportsTest() const
    {
        return test != nullptr;
    }

    // Prevent accidental copy (important for unique_ptr)
    UltrasonicDriverContext(const UltrasonicDriverContext &) = delete;
    UltrasonicDriverContext &operator=(const UltrasonicDriverContext &) = delete;

    // Allow move (safe transfer of ownership)
    UltrasonicDriverContext(UltrasonicDriverContext &&) = default;
    UltrasonicDriverContext &operator=(UltrasonicDriverContext &&) = default;

    // Default constructor
    UltrasonicDriverContext() = default;

    // Optional: explicit reset for controlled teardown
    void reset()
    {
        test = nullptr;
        driver.reset();
        receiver.reset();
    }
};
