#include "ThreadUtils.hh"
#include <cstring>
#include "Represent.h"
#include "Log.h"
#include "RWLock.hh"

namespace m3
{

int SetRealtime(int _policy, int _priority)
{
    pthread_attr_t attr;
    int policy;
    pthread_attr_init(&attr);
    iferr(pthread_attr_getschedpolicy(&attr, &policy))
    {
        logError("set_realtime: "
            "Get current scheduling policy failed 1");
        pthread_attr_destroy(&attr);
        return 1;
    }
    if(policy != SCHED_OTHER)
    {
        pthread_attr_destroy(&attr);
        return -1;
    }
    iferr(pthread_attr_setschedpolicy(&attr, _policy))
    {
        logError("set_realtime: Set scheduling policy failed 2");
        pthread_attr_destroy(&attr);
        return 2;
    }
    iferr(pthread_attr_getschedpolicy(&attr, &policy) || policy != _policy)
    {
        logError("set_realtime: Set scheduling policy failed 3");
        pthread_attr_destroy(&attr);
        return 3;
    }
    struct sched_param param;
    param.sched_priority = _priority;
    iferr(pthread_attr_setschedparam(&attr, &param))
    {
        logError("set_realtime: Set scheduling priority failed 4");
        pthread_attr_destroy(&attr);
        return -2;
    }
    pthread_attr_destroy(&attr);
    return 0;
}

static int *cpus = nullptr;
static int ncpu = 0;

int AssignCPU()
{
    static RWLock lock;
    lock.writeLock();
    if(!cpus)
    {
        ncpu = sysconf(_SC_NPROCESSORS_CONF);
        cpus = snew int[ncpu];
        memset(cpus, 0, ncpu * sizeof(int));
    }
    cpu_set_t mask;
    pthread_t self = pthread_self();
    // iferr(pthread_getaffinity_np(self, sizeof(mask), &mask) < 0)
    // {
    //     logError("AssignCPU: Failed getting affinity");
    //     lock.writeRelease();
    //     return -1;
    // }
    CPU_ZERO(&mask);
    int i = ncpu - 1;
    for(; i >= 0; i--)
    {
        if(cpus[i] == 0)
        {
            CPU_SET(i, &mask);
            cpus[i] = 1;
            break;
        }
    }
    iferr(i < 0)
    {
        logError("AssignCPU: No free core for real-time thread");
        lock.writeRelease();
        return -2;
    }
    iferr(pthread_setaffinity_np(self, sizeof(mask), &mask) < 0)
    {
        logError("AssignCPU: Failed to set affinity");
        lock.writeRelease();
        return -3;
    }
    logDebug("AssignCPU: Assign to CPU %d", i);
    lock.writeRelease();
    return i;
}

}
