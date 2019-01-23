#include "Log.h"
#include "Represent.h"
#include "PolicyManager.hh"
#include "RRScheduler.hh"
#include "LowRTTScheduler.hh"
#include "HighThroughputScheduler.hh"
#include "FinalScheduler.hh"

namespace m3
{

int PolicyManager::assignDefaultPolicy(Connection *conn, Policy::Priority prio)
{
    Policy *policy = conn->getPolicy();
	Scheduler *sched = NULL;

	/*switch (prio)
	{
	case m3::Policy::REALTIME:
		sched = snew RRScheduler();
		break;
	case m3::Policy::HIGH:
		sched = snew LowRTTScheduler();
		break;
	case m3::Policy::MEDIUM:
		sched = snew HighThroughputScheduler();
		break;
	case m3::Policy::LOW:
		sched = snew FinalScheduler();
		break;
	case m3::Policy::count:
		break;
	default:
		break;
	}*/
    sched = snew FinalScheduler();

    TCPCallback *onACK = &DumbTCPCallback::instance;
    TCPCallback *onRetx = &DumbTCPCallback::instance;
    int ret;
    iferr (sched == 0 || onACK == 0 || onRetx == 0)
    {
        logAllocError("PolicyManager::assignPolicy");
        return 1;
    }
    iferr ((ret = sched->init(cm, conn)) != 0)
    {
        logStackTrace("PolicyManager::assignPolicy", ret);
        delete sched;
        return 2;
    }

    policy->writeLock();
    policy->setScheduler(sched);
    policy->setOnACK(onACK);
    policy->setOnRetx(onRetx);
    policy->setPrio(prio);
    int len;
    Interface** interfaces = im->getAllInterfaces(&len);
    for (int i = 0; i < len; ++i)
    {
        policy->addInterface(interfaces[i]);
    }
    policy->writeRelease();

    return 0;
}

int PolicyManager::assignPolicy(Connection *conn, Policy::Priority prio)
{
    return assignDefaultPolicy(conn, prio);
}

int PolicyManager::modifyPolicy(Connection *conn, Message *msg)
{
    if (conn->getPolicy()->isInitialized())
    {
        return 0;
    }
    else
    {
        int ret = assignPolicy(conn);
        iferr (ret != 0)
        {
            logStackTrace("PolicyManager::modifyPolicy", ret);
        }
        return ret;
    }
}

}