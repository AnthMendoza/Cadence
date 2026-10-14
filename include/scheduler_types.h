#pragma once
#include <cstdlib>
#include <time_literal.h>
#include <array>
#include <chrono>
#include <queue>
#include <vector>

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
    std::chrono::steady_clock::time_point deadline;
    Priority priority;
};

struct DeadlineOrder {
    bool operator()(const ScheduledTask& a,const ScheduledTask& b) const{
        return a.deadline > b.deadline;
    }
};

using ScheduledTaskQueue = std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, DeadlineOrder>;


}
