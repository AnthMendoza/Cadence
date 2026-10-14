#pragma once

#include <time_literal.h>
#include <cstdint>

namespace cadence {

// Optional, user-owned timing accumulator. Synchronize concurrent reads/writes.
struct ProcessTimes {
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
        if (run_count == 0) return TimeDur{0};
        return total / run_count;
    }
};

}
