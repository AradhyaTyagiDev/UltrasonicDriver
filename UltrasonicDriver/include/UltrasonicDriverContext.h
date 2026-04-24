#pragma once

#include <memory>

#include "IUltrasonicDriver.h"

// 🔥 Clean ownership + access separation
struct UltrasonicDriverContext
{
    // Owning pointer (lifetime controlled here)
    std::unique_ptr<IUltrasonicDriver> driver;

    // Non-owning pointer (safe access)
    IUltrasonicDriverTestHook *test = nullptr;

    // Helper
    bool supportsTest() const
    {
        return test != nullptr;
    }
};