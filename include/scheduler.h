#pragma once
#include "cadence.h"
#include <vector>
#include <queue>
#include <process.h>
#include <worker.h>
#include <thread>
#include <type_traits>
#include <optional>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <limits>

namespace cadence {

struct SchedulerConfig{
    std::chrono::microseconds idle_wait_time{1000};
};

class scheduler {
    private:
    // Task IDs are stable vector indices; removed slots are never reused.
    std::atomic<bool> running = false;
    SchedulerConfig config;
    std::vector<std::shared_ptr<process>> process_list;
    std::vector<bool> process_active;
    std::mutex mtx;
    std::condition_variable cv; 
    WorkerCount worker_count;
    std::vector<worker> workers;
    std::vector<std::thread> threads;
    std::thread scheduler_thread;
    std::chrono::steady_clock::time_point start_time;

    struct protocall {
        bool operator()(const ScheduledTask& a, const ScheduledTask& b) {
            return a.deadline_ns > b.deadline_ns;
        }
    };

    std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, protocall> process_queue;



    void worker_factory(WorkerCount count);
    void seed_process_queue();
    void run();
    std::optional<ScheduledTask> get_next_deadline();
    
    public:

    void start();
    void stop();

    template<typename T>
    TaskId set_process(std::shared_ptr<T> process_instance){
        if constexpr (!std::is_base_of_v<process,T>) {
            return invalid_task_id;
        }
        if (!process_instance) return invalid_task_id;

        std::lock_guard<std::mutex> lock(mtx);
        const TaskId id = static_cast<TaskId>(process_list.size());
        std::shared_ptr<process> base_process = std::move(process_instance);
        process_list.push_back(std::move(base_process));
        process_active.push_back(true);
        if(running.load(std::memory_order_acquire)){
            const auto now = std::chrono::duration_cast<TimeDur>(std::chrono::steady_clock::now() - start_time);
            const auto interval = process_list[id]->get_interval();
            const auto first_deadline = now + interval;
            process_queue.push(ScheduledTask{
                id,
                first_deadline,
                process_list[id]->get_priority()
            });
            cv.notify_all();
        }
        
        return id;
    }

    bool remove_process(TaskId id);

    WorkerCount get_worker_count();
    WorkerCount get_thread_count();
    uint16_t get_process_count();


    bool is_running() const;

    static constexpr TaskId invalid_task_id = std::numeric_limits<TaskId>::max();

    scheduler(WorkerCount w_c): worker_count(w_c){

    }
};

}
