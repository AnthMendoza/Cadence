#include <cadence.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

namespace {

using namespace std::chrono_literals;

class heartbeat : public cadence::process{
public:
    heartbeat(): process(1s, cadence::Priority::Normal){}
    void task() override {
        static std::mutex output_mutex;
        const std::lock_guard<std::mutex> lock(output_mutex);
        std::cout << "Heartbeat task ran.\n";
    }
};

}
int main() {
    cadence::scheduler scheduler(2);

    const auto heartbeat_id = scheduler.set_process(std::make_shared<heartbeat>());
    if (heartbeat_id == cadence::scheduler::invalid_task_id) {
        std::cerr << "Could not register heartbeat task.\n";
        return 1;
    }

    scheduler.start();

    scheduler.notify(heartbeat_id);

    std::this_thread::sleep_for(3s);

    scheduler.remove_process(heartbeat_id);
    scheduler.stop();
}