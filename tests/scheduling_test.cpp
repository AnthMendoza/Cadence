#include <gtest/gtest.h>
#include <cadence.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;

class gate_process final : public cadence::process {
public:
    std::mutex mutex;
    std::condition_variable cv;
    bool started = false;
    bool released = false;

    gate_process() : process(10s, cadence::Priority::Normal) {}

    void task() override {
        std::unique_lock<std::mutex> lock(mutex);
        started = true;
        cv.notify_all();
        cv.wait(lock, [this] { return released; });
    }
};

class ordered_process final : public cadence::process {
public:
    const int value;
    std::vector<int>& order;
    std::mutex& order_mutex;
    std::condition_variable& order_cv;

    ordered_process(cadence::TimeDur interval, int value_, std::vector<int>& order_,
                    std::mutex& mutex_, std::condition_variable& cv_)
        : process(interval, cadence::Priority::Normal), value(value_), order(order_),
          order_mutex(mutex_), order_cv(cv_) {}

    void task() override {
        {
            std::lock_guard<std::mutex> lock(order_mutex);
            order.push_back(value);
        }
        order_cv.notify_all();
    }
};

class counting_process final : public cadence::process {
public:
    std::atomic<int> runs{0};

    explicit counting_process(cadence::TimeDur interval)
        : process(interval, cadence::Priority::Normal) {}

    void task() override {
        runs.fetch_add(1, std::memory_order_relaxed);
        std::this_thread::sleep_for(1ms);
    }
};

bool wait_for_count(const std::atomic<int>& count, int target,std::chrono::steady_clock::duration timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while(count.load(std::memory_order_relaxed) < target && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(1ms);
    }
    return count.load(std::memory_order_relaxed) >= target;
}

}

TEST(schedulingTest, runsEarliestAbsoluteDeadlineFirstWhenDeadlinesAreOverdue) {
    cadence::scheduler scheduler(1);
    auto gate = std::make_shared<gate_process>();
    const auto gate_id = scheduler.set_process(gate);
    ASSERT_NE(gate_id, cadence::scheduler::invalid_task_id);

    scheduler.start();
    const bool notified = scheduler.notify(gate_id);
    if(!notified) {
        scheduler.stop();
        FAIL() << "failed to notify the gate task";
        return;
    }

    bool gate_started;
    {
        std::unique_lock<std::mutex> lock(gate->mutex);
        gate_started = gate->cv.wait_for(lock, 1s, [&] { return gate->started; });
    }
    if(!gate_started) {
        scheduler.stop();
        FAIL() << "gate task did not start";
        return;
    }

    std::vector<int> order;
    std::mutex order_mutex;
    std::condition_variable order_cv;
    auto earlier = std::make_shared<ordered_process>(40ms, 1, order, order_mutex, order_cv);
    auto later = std::make_shared<ordered_process>(100ms, 2, order, order_mutex, order_cv);
    const auto earlier_id = scheduler.set_process(earlier);
    const auto later_id = scheduler.set_process(later);

    // Keep the only worker busy until both scheduled deadlines have passed.
    std::this_thread::sleep_for(180ms);
    {
        std::lock_guard<std::mutex> lock(gate->mutex);
        gate->released = true;
    }
    gate->cv.notify_all();

    bool got_both = false;
    {
        std::unique_lock<std::mutex> lock(order_mutex);
        got_both = order_cv.wait_for(lock, 1s, [&] { return order.size() >= 2; });
    }
    scheduler.stop();
    EXPECT_NE(earlier_id, cadence::scheduler::invalid_task_id);
    EXPECT_NE(later_id, cadence::scheduler::invalid_task_id);
    ASSERT_TRUE(got_both);
    EXPECT_EQ(order[0], 1);
    EXPECT_EQ(order[1], 2);
}

TEST(schedulingTest, longestIntervalKeepsMakingProgressWithFourWorkers) {
    cadence::scheduler scheduler(4);
    auto fast_a = std::make_shared<counting_process>(1ms);
    auto fast_b = std::make_shared<counting_process>(2ms);
    auto fast_c = std::make_shared<counting_process>(3ms);
    auto slowest = std::make_shared<counting_process>(20ms);

    scheduler.set_process(fast_a);
    scheduler.set_process(fast_b);
    scheduler.set_process(fast_c);
    scheduler.set_process(slowest);
    scheduler.start();

    const bool progressed = wait_for_count(slowest->runs, 5, 1s);
    scheduler.stop();

    EXPECT_TRUE(progressed) << "longest-interval task stopped making progress";
    EXPECT_GT(fast_a->runs.load(std::memory_order_relaxed), 0);
    EXPECT_GT(fast_b->runs.load(std::memory_order_relaxed), 0);
    EXPECT_GT(fast_c->runs.load(std::memory_order_relaxed), 0);
}
