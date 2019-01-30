#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>

#include "Connection.hh"
#include "ConnectionManager.hh"
#include "Log.h"
#include "Message.hh"
#include "NetworkUtils.h"
#include "Policy.hh"
#include "Represent.h"
#include "UserDataMessage.hh"

namespace m3
{

int ConnectionManager::init(void *arg)
{
    InitObject *iarg = (InitObject*)arg;

    for (int i = 0; i < Policy::Priority::count; ++i)
    {
        if ((handlers[i] = iarg->handlers[i]) == 0)
        {
            logError("ConnectionManager::init: Handler for prio %d not given.",
                i);
            return 1;
        }
    }

    iferr ((ct = snew ConnectionTracker(this, iarg->unit)) == 0)
    {
        logAllocError("ConnectionManager::init");
        return 2;
    }

    iferr ((metaFactory = Reflect::getCreateMethod(iarg->connMetaType)) == 0)
    {
        logError("ConnectionManager::init: Unrecognized connection metadata "
            "type %s", iarg->connMetaType);
        return 3;
    }
    
    return 0;
}

int ConnectionManager::addConnectionNoTrack(Connection *conn)
{
    Policy::Priority prio;
    int ret;

    conn->reference();

    lock.writeLock();
    connTable.insert(conn);
    lock.writeRelease();

    prio = conn->getPolicy()->getPrio();

//#ifdef ENABLE_DEBUG_LOG
    char src[32];
    char dst[32];
    logMessage("ConnectionManager::addConnection: "
        "%s:%d <-> %s:%d, prio %d", 
        inet_ntoa_r(conn->getSource()->sin_addr, src), 
        ntohs(conn->getSource()->sin_port),
        inet_ntoa_r(conn->getDest()->sin_addr, dst), 
        ntohs(conn->getDest()->sin_port),
        prio);
//#endif

    if ((ret = handlers[prio]->addConnection(conn)) != 0)
    {
        logStackTrace("ConnectionManager::insertConnection", ret);
        return 1;
    }

    logMessage("ConnectionManager::insertConnection: "
        "Insert %lx(hash %lx), new table size %d, new list[%d] size %d", 
        (unsigned long)conn, conn->getHashvalue(), (int)connTable.size(), prio,
        handlers[prio]->size());
    
    return 0;
}

int ConnectionManager::addConnection(Connection* conn)
{
    addConnectionNoTrack(conn);
    ct->addConnection(conn);

    return 0;
}

int ConnectionManager::insertConnection(
    DataMessageHeader *dh, Connection **dest)
{
    Connection *conn = snew Connection(dh);
    int ret;

    iferr (conn == 0)
    {
        logAllocError("ConnectionManager::insertConnection");
        return 1;
    }
    logDebug("Referenced by %lx", (unsigned long)this);

    iferr ((ret = addConnection(conn)) != 0)
    {
        logStackTrace("ConnectionManager::insertConnection", ret);
        delete conn;
        return 2;
    }

    *dest = conn;

    return 0;
}

int ConnectionManager::getConnection(Message *msg)
{
    IPPacket *pak = msg->toIPPacket();
    DataMessageHeader *dh = (DataMessageHeader*)pak->dataHeader;

    // init fake Connection
    // Why we don't implement a private default construtor that initializes 
    // nothing for Connection: we don't want to call it's destructor when 
    // this method returns.
    unsigned long connbuf[(sizeof(Connection) + 7) >> 3];
    ConnectionMetadataBase meta;
    Connection *conn = (Connection*)connbuf;
    conn->meta = &meta;    
    meta.init(dh);

    int res = 0;
    int ret;
    bool flag;

    lock.readLock();
    logDebug("ConnectionManager::getConnection: Current table size %d", 
        (int)connTable.size());
    auto ite = connTable.find(conn);
    flag = ite == connTable.end();
    lock.readRelease();

    // connection not found?
    iferr (flag)
    {
        Connection *ins;
        // we initialize a new connection only when the packet is a SYN packet.
        // this is essential to avoid re-opening connections on TIME_WAIT state.
        ifsucc (dh->syn)
        {
            ifsucc ((ret = insertConnection(dh, &ins)) == 0)
            {
                iferr ((ret = msg->setConnection(ins)) != 0)
                {
                    logStackTrace("ConnectionManager::getConnection", ret);
                    res = 1;
                }
            }
            else
            {
                logStackTrace("ConnectionManager::getConnection", ret);
                res = 2;
            }
        }
        else
        {
            logError("ConnectionManager::getConnection: Not a SYN packet, "
                "refuse to inject(hash %lx).", conn->getHashvalue());
            res = 3;
        }
    }
    else 
    {
        iferr ((ret = msg->setConnection(*ite)) != 0)
        {
            logStackTrace("ConnectionManager::getConnection", ret);
            res = 4;
        }
    }

    return res;
}

int ConnectionManager::next(DataMessage **dest)
{
    Connection *conn = 0;

    for (int i = 0; i < Policy::Priority::count; ++i)
    {
        if (handlers[i]->next(&conn) == 0 && conn->next(dest) == 0)
        {
            conn->getMeta()->renewOnReorderSchedule(*dest);
            return 0;
        }
    }

    return 1;
}

void ConnectionManager::ConnectionTracker::tmain(
    ConnectionTracker *ct, ConnectionManager *connmgr, timespec unit)
{
    logDebug("ConnectionManager::ConnectionTracker::tmain: Thread identify");

    while (true)
    {
        bool gc[Policy::Priority::count] = { false };
        ct->lock.writeLock();

        logMessage("ConnectionManager::ConnectionTracker::tmain: Size %lu",
            ct->connList.size());
        
        for (auto ite = ct->connList.begin(); ite != ct->connList.end();)
        {
            Connection *conn = *ite;
            TCPStateMachine *tcpsm = conn->getStateMachine();
            
            logDebug("TRACK %lx(hash %lx): %s, expire %d",
                (unsigned long)conn, conn->getHashvalue(), 
                TCPStateMachine::getStateString(tcpsm->getState()),
                tcpsm->getExpire() - 1);

            if (tcpsm->reduceExpire() == 0)
            {
                if (tcpsm->getState() == TCPStateMachine::State::TIME_WAIT)
                {
                    Policy::Priority prio = conn->getPolicy()->getPrio();
                    logDebug("ConnectionManager::ConnectionTracker::tmain: "
                        "TIME_WAIT connection %lx expired, transform to CLOSED",
                        (unsigned long)conn);
                    tcpsm->setState(TCPStateMachine::stateInvMap[
                        TCPStateMachine::State::CLOSED]);
                    gc[prio] |= (connmgr->removeConnection(conn) != 0);
                }
                else
                {
                    logDebug("ConnectionManager::ConnectionTracker::tmain: "
                        "Remove Connection %lx", (unsigned long)conn);

                    ite = ct->connList.erase(ite);
                    conn->release();
                    logDebug("Released by %lx", (unsigned long)ct);
                    continue;
                }
            }
            ite++;
        }

        ct->lock.writeRelease();
        
        for (int i = 0; i < Policy::Priority::count; ++i)
        {
            if (gc[i])
            {
                connmgr->handlers[i]->gc();
            }
        }
        nanosleep(&unit, 0);
    }
}

}
