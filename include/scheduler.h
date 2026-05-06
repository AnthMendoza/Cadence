#pragma once
#include <vector>
#include <queue>
#include <process.h>
#include <worker.h>
#include <thread>
#include <type_traits>
#include <optional>

namespace cadence {

class scheduler {
    private:
    //ID matches position in vector ID:1 = process_list[1];
    bool running = false;
    std::vector<process*> process_list;
    std::mutex mtx;
    std::condition_variable cv; 
    WorkerCount worker_count;
    std::vector<worker> workers;
    std::vector<std::thread> threads;
    std::thread scheduler_thread;
    std::__1::chrono::steady_clock::time_point start_time;

    struct protocall {
        bool operator()(const ScheduledTask& a, const ScheduledTask& b) {
            if( a.deadline_us < b.deadline_us) return true;
            return false;
        }
    };

    std::priority_queue<ScheduledTask, std::vector<ScheduledTask>, protocall> process_queue;



    void worker_factory(WorkerCount count);
    void run();
    std::optional<ScheduledTask> get_next_deadline();
    
    public:

    void start();
    void stop();

    template<typename T>
    void set_process(T& process_instance){
        if(std::is_base_of_v<process,T>){
            process_list.push_back(dynamic_cast<process*>(&process_instance));
        }
    }

    WorkerCount get_worker_count();
    WorkerCount get_thread_count();
    uint16_t get_process_count();


    bool is_running() const;

    scheduler(WorkerCount w_c): worker_count(w_c){

    }
};

}