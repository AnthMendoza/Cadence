#pragma once
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <memory>
#include <vector>
#include <process.h>
#include <optional>
#include <scheduler_types.h>

namespace cadence{

class worker {
private:
    std::mutex& mtx;
    std::condition_variable& cv;
    std::vector<process*>& process_list;

    bool running = false;
    bool wake_worker = false;
    TaskId process_id{};

public:
    worker(std::mutex& m,
           std::condition_variable& c,
           std::vector<process*>& p)
        : mtx(m), cv(c), process_list(p) {}

    void start();
    void stop();
    const bool is_running() const;
    void set_ID(TaskId id);
};
}