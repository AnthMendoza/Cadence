#pragma once
#include <vector>
#include <queue>
#include <process.h>
#include <worker.h>
#include <thread>

namespace cadence {

class scheduler {
    private:
    //ID matches position in vector ID:1 = process_list[1];
    std::vector<process*> process_list;
    std::mutex mtx;
    std::condition_variable cv; 
    WorkerCount worker_count;
    std::vector<worker> workers;
    std::vector<std::thread> threads;
    bool running = false;

    void worker_factory(WorkerCount count);
    
    public:

    void start();
    void stop();
    WorkerCount get_worker_count();
    WorkerCount get_thread_count();
    bool is_running() const;

    scheduler(uint8_t w_c):worker_count(w_c){

    }
};

}