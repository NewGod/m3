#ifndef __POLICYMANAGER_HH__
#define __POLICYMANAGER_HH__

#include "Connection.hh"
#include "ContextManager.hh"
#include "DumbTCPCallback.hh"
#include "InterfaceManager.hh"
#include "Policy.hh"
#include "RRScheduler.hh"
#include "Scheduler.hh"
#include "TCPCallback.hh"

namespace m3
{

class PolicyManager
{
protected:
    ContextManager *cm;
    InterfaceManager *im;
public:
    virtual ~PolicyManager() {}

    inline int init(ContextManager *cm, InterfaceManager *im)
    {
        this->cm = cm;
        this->im = im;
        return 0;
    }

    int assignDefaultPolicy(Connection *conn, Policy::Priority prio = Policy::Priority::MEDIUM);

    virtual int assignPolicy(Connection *conn, Policy::Priority prio = Policy::Priority::MEDIUM);
    virtual int modifyPolicy(Connection *conn, Message *msg);
    static void* newInstance() //@ReflectPolicyManager
    {
        return snew PolicyManager();
    }
};

}

#endif
