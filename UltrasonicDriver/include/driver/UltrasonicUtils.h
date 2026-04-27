// UltrasonicUtils.h

#pragma once

#include <cstddef>
#include "UltrasonicSensorTypes.h"

inline size_t toIndex(UltrasonicSensorId s)
{
    return static_cast<size_t>(s);
}