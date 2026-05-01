#include "scheduler.h"

namespace cadence{
    void scheduler::start(){
        if(running){
            return;
        }

        running = true;

        worker_factory(worker_count);
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

        for (WorkerCount i = 0 ; i < threads.size() ; i++) {
            if (threads[i].joinable()) {
                threads[i].join();
            }
        }

        workers.clear();
        workers.shrink_to_fit();
    }

    WorkerCount scheduler::get_worker_count(){
        std::lock_guard<std::mutex> lock(mtx);
        return workers.size();
    }
    WorkerCount scheduler::get_thread_count(){
        std::lock_guard<std::mutex> lock(mtx);
        return threads.size();
    }
    bool scheduler::is_running() const{
        return running;
    }

}
