#include <gtest/gtest.h>
#include <cadence.h>
#include <memory>
#include <cmath>


class timing_fixture : public cadence::process{
public:
    int ID;
    int run_count{0};
    int deadline_misses{0};
    const cadence::TimeDur epsilon{100'000};
    cadence::TimeDur max_deadline_miss_ns{0};
    std::vector<std::chrono::steady_clock::time_point> times;
    

    timing_fixture(cadence::TimeDur interval_ns , cadence::Priority prio , int ID_ ,bool logging_ = false): cadence::process(interval_ns , prio, logging_) , ID(ID_){
        times.reserve(10000);
    }

    void task() override{
        auto now = std::chrono::steady_clock::now();
        times.push_back(now);
        run_count++;
        if(run_count < 2){
            return;
        }
        const auto actual_interval = std::chrono::duration_cast<cadence::TimeDur>(times.back() - times[times.size() - 2]);
        const auto interval = get_interval();
        if (actual_interval > interval + epsilon) {
            ++deadline_misses;

            const auto miss_duration = actual_interval - interval;

            if (miss_duration > max_deadline_miss_ns) {
                max_deadline_miss_ns = miss_duration;
            }
        }

        
    }


    double average_period_us() const{
        if(times.size() < 2)
            return 0.0;

        double total_ns = std::chrono::duration<double,std::micro>(times.back() - times.front()).count();

        return total_ns / static_cast<double>(times.size() - 1);
    }

    double min_period_us() const{
        if(times.size() < 2)
            return 0.0;

        double min_ns = std::numeric_limits<double>::max();

        for(size_t i = 1; i < times.size(); ++i){
            double period = std::chrono::duration<double,std::micro>(times[i] - times[i - 1]).count();
            min_ns = std::min(min_ns, period);
        }

        return min_ns;
    }

    double max_period_us() const{
        if(times.size() < 2)
            return 0.0;

        double max_ns = 0.0;

        for(size_t i = 1; i < times.size(); ++i){
            double period =
            std::chrono::duration<double,std::micro>(times[i] - times[i - 1]).count();

            max_ns = std::max(max_ns, period);
        }

        return max_ns;
    }

    double period_stddev_us() const{
        if(times.size() < 3)
            return 0.0;

        const double mean = average_period_us();

        double sum_squared_error = 0.0;

        for(size_t i = 1; i < times.size(); ++i){
            double period = std::chrono::duration<double,std::micro>(times[i] - times[i - 1]).count();

            double error = period - mean;
            sum_squared_error += error * error;
        }

        double variance =
            sum_squared_error / static_cast<double>(times.size() - 1);

        return std::sqrt(variance);
    }

    void print() const{
        std::cout << "Process " << ID << '\n';
        std::cout << "-------------------------\n";
        std::cout << "Executions:       " << run_count << '\n';
        std::cout << "times.size(): " << times.size() << '\n';
        std::cout << "Average period:   " << average_period_us() << " us\n";
        std::cout << "Std deviation:    " << period_stddev_us() << " us\n";
        std::cout << "Deadline misses:    " << deadline_misses << "\n";
        std::cout << "Max deadline miss:   " << std::chrono::duration_cast<std::chrono::microseconds>(max_deadline_miss_ns).count() << " us\n";
    }
};



TEST(timing_tests, largeSet) {
    cadence::WorkerCount count = 4;
    cadence::scheduler sch(count);

    constexpr int num_processes = 100;
    auto interval = cadence::TimeDur(1'000'000);
    int cycles = 10000;

    std::vector<std::shared_ptr<timing_fixture>> processes;
    processes.reserve(num_processes);

    for (int i = 0; i < num_processes; ++i) {
        processes.push_back(std::make_shared<timing_fixture>(interval, cadence::Priority::Normal, i , true));
        sch.set_process(processes.back());
    }
    

    sch.start();
    std::this_thread::sleep_for(interval * cycles);
    sch.stop();

    for (const auto& process : processes) {
        process->print();
    }
}
