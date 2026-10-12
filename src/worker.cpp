#include "worker.h"

namespace cadence {

void worker::start() {
    std::unique_lock<std::mutex> lock(mtx);

    while(running && scheduler_running.load(std::memory_order_acquire)){
        cv.wait(lock, [this]{
            return !running || !scheduler_running.load(std::memory_order_acquire) || !process_queue.empty();
        });

        if(!running || !scheduler_running.load(std::memory_order_acquire)) break;


        while(!process_queue.empty()){
            const auto id = process_queue.top().ID;
            if (id < process_active.size() && process_active[id] && process_list[id]) break;
            process_queue.pop();
        }

        if (process_queue.empty()) continue;

        auto next = process_queue.top();
        const auto deadline = next.deadline;
        if (std::chrono::steady_clock::now() < deadline) {
            cv.wait_until(lock, deadline);
            continue;
        }

        process_queue.pop();
        if (next.ID >= process_list.size() || !process_active[next.ID]) continue;
        auto current = process_list[next.ID];
        if (!current) continue;
        auto process_mutex = process_mutexes[next.ID];

        lock.unlock(); //unlock before task
        std::unique_lock<std::mutex> process_lock(*process_mutex);
        current->task();
        process_lock.unlock();

        const auto interval = current->get_interval();
        const auto now = std::chrono::steady_clock::now();
        if (interval > TimeDur::zero()) {
            next.deadline += interval;
            while (next.deadline <= now) next.deadline += interval;
        }

        lock.lock(); //relock after task
        if (    running && scheduler_running.load(std::memory_order_acquire) &&
                next.ID < process_active.size() && process_active[next.ID] &&
                process_list[next.ID] && interval > TimeDur::zero()) {
            process_queue.push(next);
            cv.notify_all();
        }
    }
}

void worker::stop() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        running = false;
    }
    cv.notify_all();
}

const bool worker::is_running() const {
    std::lock_guard<std::mutex> lock(mtx);
    return running;
}

} 
