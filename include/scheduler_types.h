#pragma once
#include <cstdlib>
#include <time_literal.h>

namespace cadence{
using WorkerCount = uint16_t;
using TaskId = uint32_t;

enum class Priority : uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

struct ScheduledTask{
    TaskId ID;
    TimeUs deadline_us;
    Priority priority;
};
}