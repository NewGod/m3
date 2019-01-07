#ifndef __RRSCHEDULER_HH__
#define __RRSCHEDULER_HH__

#include "Policy.hh"
#include "Scheduler.hh"

namespace m3
{

class RRScheduler : public Scheduler
{
public:
    RRScheduler() : ptr(0) {}

    int schedule(Interface **dest, Message *msg) override
    {
        int len;
        Policy *policy = this->conn->getPolicy();
        Interface **avail = policy->acquireInterfaces(&len);
        int prev = ptr;
        int res = 0;

        do
        {
            if (avail[ptr]->isReady())
            {
                dest[0] = avail[ptr];
                res = 1;
                break;
            }
        }
        while ((ptr = ptr + 1 == len ? 0 : ptr + 1) != prev);

        policy->readRelease();

        iferr (res == 0)
        {
            logError("RRScheduler::schedule: No interface available.");
        }

        return res;
    }
    static void* newInstance() //@ReflectRRScheduler
    {
        return snew RRScheduler();
    }
protected:
    int ptr;
};

}

#endif
