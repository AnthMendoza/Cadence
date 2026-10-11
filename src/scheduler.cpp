#include "scheduler.h"
#include <algorithm>

namespace cadence{
    
    constexpr TimeDur scheduler_tick_us = TimeDur(100);

    void scheduler::start(){
        {
            std::lock_guard<std::mutex> lock(mtx);
            if(running.load(std::memory_order_acquire)) return;
            start_time = std::chrono::steady_clock::now();
            process_queue = {};
            for(TaskId i = 0; i < process_list.size(); ++i){
                if (!process_active[i]) continue;
                process_queue.push(ScheduledTask{
                    i, process_list[i]->get_interval(), process_list[i]->get_priority()
                });
            }
            running.store(true, std::memory_order_release);
        }//mutex unlocks
        worker_factory(worker_count);
        scheduler_thread = std::thread(&scheduler::run,this);
    }

    void scheduler::run() {
        while (running.load(std::memory_order_acquire)){
            auto deadline = get_next_deadline();

            if(!deadline.has_value()){
                std::unique_lock<std::mutex> lock(mtx);

                cv.wait_for(lock, config.idle_wait_time);
                continue;
            }

            auto abs_deadline = start_time + deadline->deadline_ns;
            if (std::chrono::steady_clock::now() < abs_deadline){
                std::unique_lock<std::mutex> lock(mtx);
                const auto status = cv.wait_until(lock, abs_deadline);
                if(!running.load(std::memory_order_acquire)){
                    break;
                }
                //Std::cv_status::timeout checks the reason why the cv was woken. IN this case if its time to run a task.
                if(status != std::cv_status::timeout){
                    if (deadline->ID < process_active.size() && process_active[deadline->ID]){
                        process_queue.push(*deadline);
                    }
                    continue;
                }
            }

            std::shared_ptr<process> current;
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (deadline->ID < process_list.size() && process_active[deadline->ID]){
                    current = process_list[deadline->ID];
                }
            }
            if(current == nullptr) continue;

            current->task();

            const auto now_ns = std::chrono::duration_cast<TimeDur>(std::chrono::steady_clock::now() - start_time);
            
            deadline->deadline_ns += current->get_interval();
            
            while(deadline->deadline_ns <= now_ns){
                deadline->deadline_ns += current->get_interval();
            }

            std::lock_guard<std::mutex> lock(mtx);
            if (deadline->ID < process_active.size() && process_active[deadline->ID]){
                process_queue.push(*deadline);
            }
        }
    }

    std::optional<ScheduledTask> scheduler::get_next_deadline(){
        std::lock_guard<std::mutex> lock(mtx);
        if(process_queue.empty()) return std::nullopt;
        auto next  = process_queue.top();
        process_queue.pop();
        return next;
    }

    void scheduler::seed_process_queue() {
        std::lock_guard<std::mutex> lock(mtx);
        process_queue = {};

        for(TaskId i = 0; i < process_list.size(); ++i){
            if (!process_active[i]) continue;
            process_queue.push(ScheduledTask{
                i,
                process_list[i]->get_interval(),
                process_list[i]->get_priority()
            });
        }
    }
    
    void scheduler::worker_factory(WorkerCount count){
        workers.reserve(count);
        threads.reserve(count);

        for(WorkerCount i = 0; i < count; i++){
            workers.emplace_back(mtx, cv, process_list);
        }

        for(WorkerCount i = 0; i < count; i++){
            threads.emplace_back(&worker::start, &workers[i]);
        }
    }

    void scheduler::stop(){
        running.store(false,std::memory_order_seq_cst);
        cv.notify_all();

        if(scheduler_thread.joinable()){
            scheduler_thread.join();
        }

        for(WorkerCount i = 0 ; i < workers.size() ; i++){
            workers[i].stop();
        }

        for(WorkerCount i = 0 ; i < threads.size() ; i++){
            if(threads[i].joinable()){
                threads[i].join();
            }
        }

        workers.clear();
        workers.shrink_to_fit();

        threads.clear();
        threads.shrink_to_fit();

        std::lock_guard<std::mutex> lock(mtx);
        process_queue = {};
    }

    WorkerCount scheduler::get_worker_count(){
        std::lock_guard<std::mutex> lock(mtx);
        return workers.size();
    }
    WorkerCount scheduler::get_thread_count(){
        std::lock_guard<std::mutex> lock(mtx);
        return threads.size();
    }
    uint16_t scheduler::get_process_count(){
        std::lock_guard<std::mutex> lock(mtx);
        return static_cast<uint16_t>(std::count(process_active.begin(), process_active.end(), true));
    }
    bool scheduler::is_running() const{
        return running.load(std::memory_order_relaxed);
    }

    bool scheduler::remove_process(TaskId id){
        std::lock_guard<std::mutex> lock(mtx);
        if (id >= process_active.size() || !process_active[id]) return false;
        process_active[id] = false;
        process_list[id].reset();
        cv.notify_all();
        return true;
    }

}
