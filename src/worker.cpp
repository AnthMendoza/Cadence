#include "worker.h"

namespace cadence{

    void worker::start() {
        running = true;
        std::unique_lock<std::mutex> lk(mtx);

        while (running) {
            cv.wait(lk, [this] {
                return wake_worker || !running;
            });

            if (!running) break;

            if (process_id >= process_list.size())[[unlikely]]{
                wake_worker = false;
                continue;
            }

            TaskId id = process_id;
            wake_worker = false;

            lk.unlock();
            //unlock mutex for work
            process_list[id]->run();
            lk.lock();
        }
    }

    void worker::stop(){
        running = true;
        cv.notify_one();
    }

    const bool worker::is_running() const{
        return running;
    }

    void worker::set_ID(TaskId id){
        process_id = id;
    }

}