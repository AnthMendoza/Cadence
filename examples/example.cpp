#include "scheduler.h"
#include <process.h>

class work : public cadence::process{

};

int main(){
    int worker_count = 10;
    cadence::scheduler sch(worker_count);

    return 0;
}