#include "scheduler.h"

namespace cadence{
    void scheduler::start(){
        if(running){
            return;
        }
        start_time = std::chrono::steady_clock::now();
        worker_factory(worker_count);
        scheduler_thread = std::thread(&scheduler::run,this);
        running = true;
    }

    void scheduler::run() {
        start_time = std::chrono::steady_clock::now();

        while (running) {
            auto now = std::chrono::steady_clock::now();

            TimeUs elapsed_us = static_cast<TimeUs>(std::chrono::duration_cast<std::chrono::microseconds>(now - start_time).count());
            
            auto deadline = get_next_deadline();
            
            auto abs_deadline = start_time + std::chrono::microseconds(deadline->deadline_us);

            auto procedure = [this](std::optional<ScheduledTask> task) {
                process_list[task->ID]->run();

            };

            if (std::chrono::steady_clock::now() < abs_deadline) {
                std::this_thread::sleep_until(abs_deadline);
                procedure(deadline);

            }else{
                procedure(deadline);
                //missed deadline
            }
            
            auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - start_time).count();
            do {
                deadline->deadline_us += process_list[deadline->ID]->get_interval();
            } while (deadline->deadline_us <= now_us);
        }
    }

    std::optional<ScheduledTask> scheduler::get_next_deadline(){
        if (process_queue.empty()) return std::nullopt;
        auto next  = process_queue.top();
        process_queue.pop();
        return next;
    }
    
    void scheduler::worker_factory(WorkerCount count) {
        workers.reserve(count);
        threads.reserve(count);

        for (WorkerCount i = 0; i < count; i++) {
            workers.emplace_back(mtx, cv, process_list);
        }

        for (WorkerCount i = 0; i < count; i++) {
            threads.emplace_back(&worker::start, &workers[i]);
        }
    }

    void scheduler::stop(){
        running = false;

        if(scheduler_thread.joinable()){
            scheduler_thread.join();
        }

        for (WorkerCount i = 0 ; i < workers.size() ; i++) {
            workers[i].stop();
        }
        workers.clear();
        workers.shrink_to_fit();
        for (WorkerCount i = 0 ; i < threads.size() ; i++) {
            if(threads[i].joinable()){
                threads[i].join();
            }
        }
        threads.clear();
        threads.shrink_to_fit();
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
        return process_list.size();
    }
    bool scheduler::is_running() const{
        return running;
    }

}
