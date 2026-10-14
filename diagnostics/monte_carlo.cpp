#include <cadence.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <memory>
#include <random>
#include <thread>
#include <vector>

namespace {

using namespace std::chrono_literals;

class timed_process final : public cadence::process {
public:
    const int id;
    const std::chrono::microseconds work_duration;
    int run_count{0};
    int deadline_misses{0};
    int skipped_intervals{0};
    std::vector<cadence::TimeDur> lateness_samples;
    std::chrono::steady_clock::time_point next_deadline;

    timed_process(std::chrono::milliseconds interval, int id_, std::chrono::microseconds work,
                  std::chrono::steady_clock::time_point first_deadline)
        : process(interval, cadence::Priority::Normal), id(id_), work_duration(work),
          next_deadline(first_deadline) {}

    void task() override {
        const auto now = std::chrono::steady_clock::now();
        const auto lateness = std::chrono::duration_cast<cadence::TimeDur>(now - next_deadline);
        const auto nonnegative_lateness = std::max(cadence::TimeDur::zero(), lateness);
        lateness_samples.push_back(nonnegative_lateness);
        ++run_count;
        if(work_duration > 0us) std::this_thread::sleep_for(work_duration);

        const auto completed_at = std::chrono::steady_clock::now();
        next_deadline += get_interval();
        while(next_deadline <= completed_at) {
            ++skipped_intervals;
            next_deadline += get_interval();
        }
    }
};

struct measurements {
    int runs{0};
    int misses{0};
    int skipped_intervals{0};
    std::vector<cadence::TimeDur> lateness_samples;
};

cadence::TimeDur percentile(std::vector<cadence::TimeDur> samples, double fraction) {
    if(samples.empty()) return cadence::TimeDur::zero();
    std::sort(samples.begin(), samples.end());
    const auto index = static_cast<size_t>(std::ceil(fraction * samples.size())) - 1;
    return samples[std::min(index, samples.size() - 1)];
}

}

int main() {
    constexpr int process_count = 100;
    constexpr int trial_count = 5;
    constexpr auto sample_duration = 300ms;
    const std::vector<std::chrono::milliseconds> intervals{10ms, 25ms, 50ms, 100ms, 200ms};
    const std::vector<std::chrono::microseconds> work_durations{5us, 50us, 100us, 500us, 1000us, 2000us, 5000us};
    const std::vector<cadence::WorkerCount> worker_counts{1, 2, 4, 8, 10};
    std::uniform_int_distribution<size_t> interval_pick(0, intervals.size() - 1);
    std::uniform_int_distribution<size_t> work_pick(0, work_durations.size() - 1);

    std::cout << "\nMonte Carlo mixed periodic workload (" << process_count
              << " processes, " << trial_count << " trials/configuration, "
              << std::chrono::duration_cast<std::chrono::milliseconds>(sample_duration).count()
              << " ms/trial)\n"
              << "workers trial nominal_cpu_load runs/s miss_pct skipped p50_late_us p95_late_us p99_late_us\n";

    for(const auto worker_count : worker_counts) {
        for(int trial = 0; trial < trial_count; ++trial) {
            
            std::mt19937 random(0xC0FFEEu + static_cast<unsigned>(trial));
            cadence::scheduler scheduler(worker_count);
            std::vector<std::shared_ptr<timed_process>> processes;
            processes.reserve(process_count);
            double nominal_cpu_load = 0.0;

            scheduler.start();
            for(int i = 0; i < process_count; ++i) {
                const auto process_interval = intervals[interval_pick(random)];
                const auto work = work_durations[work_pick(random)];
                nominal_cpu_load += static_cast<double>(work.count()) /
                    std::chrono::duration<double, std::micro>(process_interval).count();
                const auto first_deadline = std::chrono::steady_clock::now() + process_interval;
                processes.push_back(std::make_shared<timed_process>(process_interval, i, work, first_deadline));
                scheduler.set_process(processes.back());
            }

            const auto sample_start = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(sample_duration);
            scheduler.stop();
            const auto elapsed = std::chrono::steady_clock::now() - sample_start;

            measurements result;
            for(const auto& process : processes) {
                result.runs += process->run_count;
                result.misses += process->deadline_misses;
                result.skipped_intervals += process->skipped_intervals;
                result.lateness_samples.insert(result.lateness_samples.end(),
                    process->lateness_samples.begin(), process->lateness_samples.end());
            }

            const double miss_rate = result.runs == 0 ? 0.0 :
                100.0 * static_cast<double>(result.misses) / static_cast<double>(result.runs);
            const double runs_per_second = static_cast<double>(result.runs) /
                std::chrono::duration<double>(elapsed).count();

            std::cout << std::setw(7) << worker_count << ' '
                      << std::setw(6) << trial << ' '
                      << std::setw(16) << std::fixed << std::setprecision(2) << nominal_cpu_load << ' '
                      << std::setw(6) << std::setprecision(1) << runs_per_second << ' '
                      << std::setw(13) << miss_rate << ' '
                      << std::setw(7) << result.skipped_intervals << ' '
                      << std::setw(12) << std::chrono::duration<double, std::micro>(percentile(result.lateness_samples, 0.50)).count() << ' '
                      << std::setw(12) << std::chrono::duration<double, std::micro>(percentile(result.lateness_samples, 0.95)).count() << ' '
                      << std::setw(12) << std::chrono::duration<double, std::micro>(percentile(result.lateness_samples, 0.99)).count()
                      << '\n';
        }
    }
}
