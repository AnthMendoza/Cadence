#pragma once
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <memory>
#include <vector>
#include <process.h>
#include <scheduler_types.h>

namespace cadence{

class worker {
private:
    std::mutex& mtx;
    std::condition_variable& cv;
    std::vector<std::shared_ptr<process>>& process_list;
    std::vector<bool>& process_active;
    std::vector<std::shared_ptr<std::mutex>>& process_mutexes;
    ScheduledTaskQueue& process_queue;
    std::atomic<bool>& scheduler_running;

    bool running = true;

public:
    worker( std::mutex& m,
            std::condition_variable& c,
            std::vector<std::shared_ptr<process>>& p,
            std::vector<bool>& pa,
            std::vector<std::shared_ptr<std::mutex>>& pm,
            ScheduledTaskQueue& q,
            std::atomic<bool>& scheduler_state)
        : mtx(m), cv(c), process_list(p), process_active(pa), process_mutexes(pm), process_queue(q),
          scheduler_running(scheduler_state) {}

    void start();
    void stop();
    const bool is_running() const;
};
}
