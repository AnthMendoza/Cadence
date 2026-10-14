# Cadence

A C++17 concurrent scheduler for recurring tasks and on-demand execution.

Cadence runs application-defined processes on a configurable pool of worker
threads. Each process supplies a task and an interval; the scheduler manages
deadlines, dispatch, and registration while workers execute the task body.

**Status:** in development. The current implementation supports periodic
scheduling and runtime task management. See [Current limitations](#current-limitations)
before integrating it into a production application.

## Features

- **Periodic execution:** schedule work using standard `std::chrono` durations.
- **Configurable concurrency:** choose the number of worker threads.
- **Deadline ordering:** dispatch the earliest queued deadline first using a
  monotonic `std::chrono::steady_clock`.
- **Runtime registration:** add and remove processes before or during execution.
- **Immediate notifications:** enqueue a process for execution without waiting
  for its next scheduled deadline.
- **Per-process serialization:** executions of the same registered process do
  not overlap; different processes can run concurrently.
- **Explicit lifecycle:** start, stop, and restart the scheduler with stable task IDs.

## Build and run

### Requirements

- A C++17-capable compiler.
- CMake 4.2 or newer, as required by the current build configuration.
- A native build tool supported by CMake, such as Make, Ninja, or MSBuild.
- Network access on initial configuration when tests are enabled: CMake fetches
  GoogleTest [v1.18.0](https://github.com/google/googletest/releases/tag/v1.18.0)
  from GitHub and verifies the archive's SHA-256 checksum. Builds with tests
  disabled do not require GoogleTest.

```sh
git clone https://github.com/AnthMendoza/Cadence.git
cd Cadence
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Run the included heartbeat example:

```sh
./build/example
```

For multi-configuration generators, the executable is typically under
`build/Release/` (for example, `build/Release/example.exe` on Windows).

The default build produces the `Cadence_lib` static library, the `example`
executable, and the `Cadence_tests` functional test executable. Monte Carlo
characterization is opt-in and runs separately from the test suite.

To build without tests or the GoogleTest download:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build --config Release --parallel
```

| CMake option | Default | Purpose |
| --- | --- | --- |
| `BUILD_TESTING` | `ON` | Build and register functional tests with CTest. |
| `CADENCE_BUILD_CHARACTERIZATION` | `OFF` | Build the independent Monte Carlo runner. Does not require GoogleTest. |

## Quick start

Derive from `cadence::process`, implement `task()`, and register a shared instance
with a scheduler:

```cpp
#include <cadence.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

using namespace std::chrono_literals;

class heartbeat final : public cadence::process {
public:
    std::atomic<unsigned> ticks{0};

    heartbeat() : process(1s, cadence::Priority::Normal) {}

    void task() override {
        ticks.fetch_add(1, std::memory_order_relaxed);
    }
};

int main() {
    cadence::scheduler scheduler(2);
    const auto task = std::make_shared<heartbeat>();
    const auto id = scheduler.set_process(task);
    if (id == cadence::scheduler::invalid_task_id) {
        return 1;
    }

    scheduler.start();
    std::this_thread::sleep_for(3s);
    scheduler.remove_process(id);
    scheduler.stop();
}
```

Call `scheduler.notify(id)` after `start()` to enqueue immediate execution.
With the current implementation, notifying a process with a positive interval
also creates another recurring schedule for that process. Each notification
can therefore increase its ongoing execution frequency.

## Scheduling and lifecycle

| Operation | Behavior |
| --- | --- |
| `scheduler(worker_count)` | Sets the worker count. Use at least one worker to execute tasks. |
| `set_process(shared_ptr)` | Returns a stable `TaskId`; a null pointer returns `invalid_task_id`. |
| `start()` | Starts workers and schedules each active process at the current time plus its interval. Repeated calls while running have no effect. |
| `notify(id, priority)` | Enqueues an immediate deadline; returns `false` for an invalid or removed ID. Priority currently does not affect dispatch order. |
| `remove_process(id)` | Deactivates the process and releases the scheduler's ownership. Returns `false` for an invalid or already removed ID. |
| `stop()` | Signals shutdown, waits for worker threads to finish, and clears queued work. Registered processes remain available for a later `start()`. |

For positive intervals, recurrence advances from the previous deadline. When a
task overruns its interval, missed deadlines are skipped until the next deadline
lies in the future. Tasks with a zero interval execute once when initially
scheduled and can be triggered again with `notify()`.

Removing a process does not cancel work already dispatched to a worker. Such
work may still execute or finish after removal. Removed task IDs are never reused.
Notifications queued while stopped are cleared by the next `start()`.

The scheduler destructor calls `stop()` automatically. Call `stop()` explicitly
when you need to wait for completion before reading results, and manage lifecycle
calls and destruction from an owning thread. A task should return in bounded time so shutdown can
complete. Synchronize data shared between different processes or with application
threads, and handle exceptions inside `task()`.

## Optional process timing

Timing collection belongs to the user-defined process. The scheduler and workers
do not measure task execution or update timing statistics, and the base
`cadence::process` stores no timing data. Processes that need measurements can
include `<process_times.h>` and own a `cadence::ProcessTimes` accumulator:

```cpp
#include <cadence.h>
#include <process_times.h>

#include <chrono>

class measured_process : public cadence::process {
public:
    cadence::ProcessTimes times;

    measured_process()
        : process(std::chrono::milliseconds(100), cadence::Priority::Normal) {}

    void task() override {
        const auto started = std::chrono::steady_clock::now();
        // Perform application work here.
        times.record(std::chrono::duration_cast<cadence::TimeDur>(
            std::chrono::steady_clock::now() - started));
    }
};
```

`ProcessTimes` provides `run_count`, `total`, `max`, `last`, and `avg()`.
Read these fields after `scheduler.stop()` has joined the workers, or add your
own synchronization for live access. The helper itself is not thread-safe.
Users decide which work to measure and whether to record each execution.

The former logging constructor argument and base `get_logs()` accessor have
been removed. Construct a process with its interval and priority, and store any
optional metrics in the derived class instead. `ProcessTimes` is available via
its explicit header rather than the scheduler's headers.

## CMake integration

To use a checkout as a subdirectory of another CMake project:

```cmake
add_subdirectory(external/Cadence)

target_compile_features(your_app PRIVATE cxx_std_17)
target_link_libraries(your_app PRIVATE Cadence_lib)
```

Replace `external/Cadence` with the checkout path and `your_app` with your target.
The library target exposes its include directory and thread dependency. Including
Cadence this way also configures its example. Set `BUILD_TESTING=OFF` when
configuring the parent project to omit Cadence's tests and GoogleTest download;
this standard CMake option may also control tests in the parent project.
There is currently no install/export package or library-only build option.

## Tests

The GoogleTest suite covers worker creation, repeated starts, shutdown, process
registration and removal, deadline ordering, and progress under mixed intervals.
Run the functional suite with the CTest command above. If tests were previously
disabled, reconfigure with `-DBUILD_TESTING=ON` and rebuild first.
To run only scheduling tests:

```sh
./build/Cadence_tests --gtest_filter='schedulingTest.*'
```

## Diagnostics and characterization

The standalone Monte Carlo runner measures a mixed periodic workload across
several worker counts. It prints throughput, skipped intervals, and lateness
percentiles. These measurements characterize the workload on the current machine
and are not latency guarantees or functional pass/fail assertions.

Build and run it independently, with functional tests disabled:

```sh
cmake -S . -B build-characterization -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF -DCADENCE_BUILD_CHARACTERIZATION=ON
cmake --build build-characterization --config Release \
  --target Cadence_characterization --parallel
./build-characterization/Cadence_characterization
```

For multi-configuration generators, use the executable in the `Release/`
subdirectory. The runner is never registered with CTest, even when both build
options are enabled; execute it explicitly when characterization is needed.

## Current limitations

- Priority levels (`Low`, `Normal`, `High`, `Critical`) are present in the API,
  but the queue currently compares deadlines only.
- `running_flag()` is exposed, but workers do not currently update the
  per-process running flag.
- Shutdown waits for dispatched tasks to finish; executing tasks cannot be cancelled.
- Exceptions escaping `task()` are not caught by workers and can terminate the
  application.
- Execution timing depends on operating-system scheduling and workload. Cadence
  does not provide hard real-time guarantees.

## Repository layout

```text
include/     Public headers and scheduling types
src/         Scheduler and worker implementations
examples/    Runnable usage example
tests/       Functional regression tests
diagnostics/ Standalone Monte Carlo characterization workload
```

## Contributing

Keep changes focused and describe the behavior they affect. For scheduler
changes, include a regression test where practical and run the full test suite.
Report issues with compiler and CMake versions, reproduction steps, and relevant
test output. Update usage documentation when public behavior changes.

## License

Cadence is available under the [MIT License](LICENSE).
