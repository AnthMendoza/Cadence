#include "scheduler.h"

namespace cadence{
    void scheduler::start(){

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
}
