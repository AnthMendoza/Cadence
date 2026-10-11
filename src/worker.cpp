#include "worker.h"

namespace cadence{

    void worker::start() {
        std::unique_lock<std::mutex> lk(mtx);

        while (running) {
            cv.wait(lk, [this] {
                return wake_worker || !running;
            });

            if (!running) break;

            if (process_id >= process_list.size() || !process_list[process_id])[[unlikely]]{
                wake_worker = false;
                continue;
            }

            TaskId id = process_id;
            auto current = process_list[id];
            wake_worker = false;

            lk.unlock();
            //unlock mutex for work
            current->task();
            lk.lock();
        }
    }

    void worker::stop(){
        {
            std::lock_guard<std::mutex> lk(mtx);
            running = false;
        }
        cv.notify_all();
    }

    const bool worker::is_running() const{
        std::lock_guard<std::mutex> lk(mtx);
        return running;
    }

    void worker::set_ID(TaskId id){
        process_id = id;
    }

}
