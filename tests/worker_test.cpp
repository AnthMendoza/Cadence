#include <gtest/gtest.h>
#include <cadence.h>

class worker_fixture : public cadence::process{
    public:
    int ID;
    int run_count{0};
    worker_fixture(cadence::TimeUs interval_us , cadence::Priority prio , int ID_ ): cadence::process(interval_us , prio) , ID(ID_){

    }

    void run() override{
        run_count ++;
    }

};

TEST(workerTest, registersProcesses) {
    cadence::WorkerCount count = 4;
    cadence::scheduler sch(count);

    constexpr int num_processes = 20;
    std::vector<worker_fixture> fixtures;

    for (int i = 0; i < num_processes; ++i) {
        fixtures.emplace_back(200, cadence::Priority::Normal, i);
        sch.set_process(fixtures.back());
    }

    EXPECT_EQ(sch.get_process_count(), num_processes);
}


TEST(workerTest, executesProcesses) {
    cadence::scheduler sch(4);

    worker_fixture proc(100, cadence::Priority::Normal, 1);

    sch.set_process(proc);
    sch.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    sch.stop();

    EXPECT_EQ(proc.run_count, 5);
}


TEST(workerTest, setOfProcessExecutes) {
    cadence::WorkerCount count = 4;
    cadence::scheduler sch(count);

    constexpr int num_processes = 20;
    std::vector<worker_fixture> fixtures;

    for (int i = 0; i < num_processes; ++i) {
        fixtures.emplace_back(100, cadence::Priority::Normal, i);
        sch.set_process(fixtures.back());
    }
    sch.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    sch.stop();
    for (int i = 0; i < num_processes; ++i) {
        EXPECT_EQ(fixtures[i].run_count, 5);
    }
}
