#pragma once
#include <cstdlib>
#include <time_literal.h>
#include <array>
#include <chrono>

namespace cadence{
using WorkerCount = uint16_t;
using TaskId = uint64_t;

enum class Priority : uint8_t {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

struct ScheduledTask{
    TaskId ID;
    TimeDur deadline_ns;
    Priority priority;
};


struct ProcessTimes{
    bool logging_enabled = false;
    uint64_t run_count = 0;
    TimeDur total{0};
    TimeDur max{0};
    TimeDur last{0};

    void record(TimeDur duration) {
        ++run_count;
        total += duration;
        last = duration;
        if (duration > max) max = duration;
    }

    TimeDur avg() const {
        if(run_count <= 0){
            return TimeDur{0};
        }
        return total/run_count;
    }
};

}