#ifndef __POLICY_HH__
#define __POLICY_HH__

#include <atomic>
#include <vector>

#include "DumbTCPCallback.hh"
#include "Interface.hh"
#include "Represent.h"
#include "RWLock.hh"
#include "Scheduler.hh"
#include "TCPCallback.hh"

namespace m3
{

class Policy
{
public:
    enum Priority { REALTIME = 0, HIGH, MEDIUM, LOW, count };
protected:
    std::atomic<Priority> prio;
    std::vector<Interface*> avail;
    Scheduler *sched;
    TCPCallback *onACK;
    TCPCallback *onRetx;
    RWLock lock;
public:
    Policy() : prio(Priority::MEDIUM), avail(), sched(0), 
        onACK(&DumbTCPCallback::instance),
        onRetx(&DumbTCPCallback::instance), lock() {}
    ~Policy()
    {
        if (onACK != &DumbTCPCallback::instance)
        {
            delete onACK;
        }
        if (onRetx != &DumbTCPCallback::instance)
        {
            delete onRetx;
        }
        clearptr(sched);
    }

    inline bool isInitialized()
    {
        return sched != 0;
    }
    inline Priority getPrio()
    {
        return prio.load();
    }
    inline void setPrio(Priority nprio)
    {
        prio = nprio;
    }
    inline Scheduler* acquireScheduler()
    {
        lock.readLock();
        return sched;
    }
    inline TCPCallback* acquireOnACK()
    {
        lock.readLock();
        return onACK;
    }
    inline TCPCallback* acquireOnRetx()
    {
        lock.readLock();
        return onRetx;
    }
    inline Interface** acquireInterfaces(int *len)
    {
        lock.readLock();
        *len = avail.size();
        return avail.data();
    }
    inline void readRelease()
    {
        lock.readRelease();
    }
    inline void writeLock()
    {
        lock.writeLock();
    }
    inline void writeRelease()
    {
        lock.writeRelease();
    }
    // hold write lock to call methods below.
    inline void setScheduler(Scheduler *nsched)
    {
        sched = nsched;
    }
    inline void setOnACK(TCPCallback *nonACK)
    {
        onACK = nonACK;
    }
    inline void setOnRetx(TCPCallback *nonRetx)
    {
        onRetx = nonRetx;
    }
    inline void addInterface(Interface *interface)
    {
        avail.push_back(interface);
    }
    inline void removeInterface(Interface *interface)
    {
        for (auto ite = avail.begin(); ite != avail.end(); ++ite)
        {
            if (*ite == interface)
            {
                avail.erase(ite);
                break;
            }
        }
    }
    static void* newInstance() //@ReflectPolicy
    {
        return snew Policy();
    }
};

}

#endif
