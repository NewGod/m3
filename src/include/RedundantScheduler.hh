#ifndef __REDUNDANTSCHEDULER_HH__
#define __REDUNDANTSCHEDULER_HH__

#include "Policy.hh"
#include "Scheduler.hh"

namespace m3
{

class RedundantScheduler : public Scheduler
{
public:
    int schedule(Interface **dest, Message *msg) override
    {
        int len;
        Policy *policy = this->conn->getPolicy();
        Interface **avail = policy->acquireInterfaces(&len);
        int res = 0;

        for (int i = 0; i < len; ++i)
        {
            if (avail[i]->isReady())
            {
                dest[res++] = avail[i];
            }
        }
        
        policy->readRelease();

        iferr (res == 0)
        {
            logError("RedundantScheduler::schedule: No interface available.");
        }

        return res;
    }
    
    static void* newInstance() //@ReflectRedundantScheduler
    {
        return snew RedundantScheduler();
    }
};

}

#endif
