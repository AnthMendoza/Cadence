#include "scheduler.h"
#include <algorithm>

namespace cadence{
    
    void scheduler::start(){
        {
            std::lock_guard<std::mutex> lock(mtx);
            if(running.load(std::memory_order_acquire)) return;
            const auto now = std::chrono::steady_clock::now();
            process_queue = {};
            for(TaskId i = 0; i < process_list.size(); ++i){
                if (!process_active[i]) continue;
                process_queue.push(ScheduledTask{
                    i, now + process_list[i]->get_interval(), process_list[i]->get_priority()
                });
            }
            running.store(true, std::memory_order_release);
        }//mutex unlocks
        worker_factory(worker_count);
    }

    void scheduler::worker_factory(WorkerCount count){
        workers.reserve(count);
        threads.reserve(count);

        for(WorkerCount i = 0; i < count; i++){
            workers.emplace_back(mtx, cv, process_list, process_active, process_mutexes,
                                 process_queue, running);
        }

        for(WorkerCount i = 0; i < count; i++){
            threads.emplace_back(&worker::start, &workers[i]);
        }
    }

    void scheduler::stop(){
        running.store(false,std::memory_order_seq_cst);
        cv.notify_all();

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

    //adds a task request with 0 time interval. higher priority over scheduled task will run first
    bool scheduler::notify(TaskId id,Priority p){
        std::lock_guard<std::mutex> lock(mtx);
        if(id >= process_active.size() || !process_active[id]) return false;
        process_queue.push({id,std::chrono::steady_clock::now(),p});
        cv.notify_all();
        return true;
    }

}
