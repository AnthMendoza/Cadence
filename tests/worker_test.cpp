#include <gtest/gtest.h>
#include <cadence.h>
#include <memory>


class fixture : public cadence::process{
    public:
    int ID;
    int run_count{0};
    fixture(cadence::TimeDur interval_us , cadence::Priority prio , int ID_): cadence::process(interval_us , prio) , ID(ID_){

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
