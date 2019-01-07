#include <sys/socket.h>
#include <sys/types.h>

#include "Connection.hh"
#include "DataMessageHeader.hh"
#include "Log.h"
#include "Represent.h"
#include "ServerPacketSource.hh"

namespace m3
{

Connection::~Connection()
{
    DataMessage *msg;
    ConnectionType *ptype;
    if (meta->proxyPort)
    {
        logDebug("Connection::destroy: Release port %d.", meta->proxyPort);
        ServerPacketSource::getInstance()->releasePort(meta->proxyPort);
    }
    if ((ptype = type.load()) != 0)
    {
        ptype->release();
    }
    while (outbuf.tryPurchase(&msg) == 0)
    {
        delete msg;
    }
    while (retxbuf.tryPurchase(&msg)== 0)
    {
        delete msg;
    }
    clearptr(meta);
}

int Connection::renewOnOutput(Message *message)
{
    state.transformOnOutput(message);
    meta->renewOnOutput(message);
    return 0;
}

int Connection::renewOnInput(Message *message)
{
    state.transformOnInput(message);
    meta->renewOnInput(message);
    return 0;
}

int StatelessConnection::renewOnOutput(Message *message)
{
    meta->renewOnOutput(message);
    return 0;
}

int StatelessConnection::renewOnInput(Message *message)
{
    meta->renewOnInput(message);
    return 0;
}

int Connection::schedule(Interface **ret, Message *msg)
{
    Scheduler *sched = policy.acquireScheduler();
    int res = sched->schedule(ret, msg);
    policy.readRelease();

    return res;
}

int Connection::next(DataMessage **ret)
{
    DataMessage *x;
    if (retxbuf.tryPurchase(&x) == 0)
    {
        *ret = x;
        //logMessage("PUR %lu", (unsigned long)x);
        return 0;
    }
    else if (outbuf.tryPurchase(&x) == 0)
    {
        *ret = x;
        //logMessage("PUR %lu", (unsigned long)x);
        return 0;
    }
    return 1;
}

bool Connection::hasNext()
{
    return retxbuf.hasNext() || outbuf.hasNext();
}

}