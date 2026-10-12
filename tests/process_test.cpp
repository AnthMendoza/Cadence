#include <gtest/gtest.h>
#include <cadence.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

namespace {

class counting_process : public cadence::process{
public:
    std::atomic<int> run_count{0};

    explicit counting_process(cadence::TimeDur interval)
        : cadence::process(interval, cadence::Priority::Normal) {}

    void task() override {
        run_count.fetch_add(1, std::memory_order_relaxed);
    }
};

bool wait_for_runs(const counting_process& task, int count){
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while(task.run_count.load(std::memory_order_relaxed) < count && std::chrono::steady_clock::now() < deadline){
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return task.run_count.load(std::memory_order_relaxed) >= count;
}

}

TEST(processTest, registersAndRemovesProcessBeforeStart) {
    cadence::scheduler sch(1);
    auto task = std::make_shared<counting_process>(std::chrono::milliseconds(1));
    std::weak_ptr<counting_process> task_lifetime = task;

    const auto id = sch.set_process(task);

    ASSERT_NE(id, cadence::scheduler::invalid_task_id);
    EXPECT_EQ(sch.get_process_count(), 1);
    task.reset();
    EXPECT_FALSE(task_lifetime.expired());

    EXPECT_TRUE(sch.remove_process(id));
    EXPECT_FALSE(sch.remove_process(id));
    EXPECT_EQ(sch.get_process_count(), 0);
    EXPECT_TRUE(task_lifetime.expired());
}

TEST(processTest, addsAndRemovesProcessWhileRunning) {
    cadence::scheduler sch(1);
    sch.start();

    auto task = std::make_shared<counting_process>(std::chrono::milliseconds(2));
    const auto id = sch.set_process(task);

    EXPECT_NE(id, cadence::scheduler::invalid_task_id);
    const bool ran = wait_for_runs(*task, 2);
    EXPECT_TRUE(ran) << "task added while running did not execute";
    if (!ran) {
        sch.stop();
        return;
    }
    EXPECT_EQ(sch.get_process_count(), 1);

    EXPECT_TRUE(sch.remove_process(id));
    EXPECT_EQ(sch.get_process_count(), 0);


    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const int runs_after_removal = task->run_count.load(std::memory_order_relaxed);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(task->run_count.load(std::memory_order_relaxed), runs_after_removal);

    sch.stop();
}