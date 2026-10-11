#include <gtest/gtest.h>
#include <cadence.h>
#include <memory>


class fixture : public cadence::process{
    public:
    int ID;
    int run_count{0};
    fixture(cadence::TimeDur interval_us , cadence::Priority prio , int ID_ ,bool logging_ = false): cadence::process(interval_us , prio, logging_) , ID(ID_){

    }

    void task() override{
        run_count ++;
    }

};

TEST(workerTest, registersProcesses) {
    cadence::WorkerCount count = 4;
    cadence::scheduler sch(count);

    constexpr int num_processes = 20;

    for (int i = 0; i < num_processes; ++i) {
        auto fix = std::make_shared<fixture>( cadence::TimeDur(100), cadence::Priority::Normal, i);
        sch.set_process(fix);
    }

    EXPECT_EQ(sch.get_process_count(), num_processes);
}

/*
TEST(workerTest, executesExpiredProcess) {
    cadence::WorkerCount count = 1;
    cadence::scheduler sch(count);

    auto interval = cadence::TimeDur(1'000'000);
    int cycles = 100;

    fixture proc( interval, cadence::Priority::Normal, 1);

    sch.set_process(proc);
    sch.start();



    std::this_thread::sleep_for(interval * cycles / 2);
    proc
    std::this_thread::sleep_until();


    sch.stop();

    EXPECT_EQ(proc.run_count, cycles);
}

/*

TEST(workerTest, executesProcesses) {
    cadence::WorkerCount count = 4;
    cadence::scheduler sch(count);

    auto interval = cadence::TimeDur(1'000'000);
    int cycles = 10000;

    fixture proc( interval, cadence::Priority::Normal, 1);

    sch.set_process(proc);
    sch.start();

    std::this_thread::sleep_for(interval*cycles);

    sch.stop();

    EXPECT_EQ(proc.run_count, cycles);
}


TEST(workerTest, setOfProcessExecutes) {
    cadence::WorkerCount count = 4;
    cadence::scheduler sch(count);

    constexpr int num_processes = 20;
    auto interval = cadence::TimeDur(1'000'000);
    int cycles = 10000;

    std::vector<std::shared_ptr<fixture>> processes;
    processes.reserve(num_processes);

    for (int i = 0; i < num_processes; ++i) {
        processes.push_back(std::make_shared<fixture>(interval, cadence::Priority::Normal, i , true));
        sch.set_process(processes.back());
    }

    sch.start();
    std::this_thread::sleep_for(interval * cycles);
    sch.stop();

    for (const auto& process : processes) {
        EXPECT_NEAR(process->run_count, cycles,1);
    }
}
*/
