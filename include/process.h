#pragma once
#include <time_literal.h>
#include <scheduler_types.h>
#include <atomic>

namespace cadence{



class process{
    private:

    TimeDur interval_ns;
    Priority priority;
    std::atomic<bool> is_running{false};

    protected:

    void set_interval(const TimeDur interval_us_){
        interval_ns = interval_us_;
    }

    public:
    
    const TimeDur get_interval() const{
        return interval_ns;
    }

    const Priority get_priority() const{
        return priority;
    }

    bool running_flag() const{
        return is_running.load(std::memory_order_acquire);
    }

    process() = delete;

    process(TimeDur interval , Priority priority_): interval_ns(interval) , priority(priority_){}

    process(const process& other) = delete;

    virtual void task() = 0;



};

}
