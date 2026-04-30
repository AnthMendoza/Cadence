#pragma once
#include <time_literal.h>
#include <scheduler_types.h>

namespace cadence{

class process{
    private:

    TimeUs interval_us;
    Priority priority;

    protected:

    const TimeUs get_interval() const{
        return interval_us;
    }

    void set_interval(const TimeUs interval_us_){
        interval_us = interval_us_;
    }

    public:
    process() = delete;

    process(TimeUs interval , Priority priority_): interval_us(interval) , priority(priority_){
    }
    virtual void run() = 0;
};

}