#ifndef __CONNECTIONMANAGER_HH__
#define __CONNECTIONMANAGER_HH__

#include <thread>
#include <unordered_set>

#include "Connection.hh"
#include "ConnectionSetHandler.hh"
#include "DataMessage.hh"
#include "Message.hh"
#include "PacketSource.hh"
#include "Policy.hh"
#include "Reflect.hh"
#include "RWLock.hh"

namespace m3
{

// Append packets to buffer here
class ConnectionManager
{
protected:
    class ConnectionTracker : public std::thread
    {
    protected:
        static void tmain(
            ConnectionTracker *ct,
            ConnectionManager *connmgr,
            timespec unit);
        RWLock lock;
        std::list<Connection*> connList;
    public:
        ConnectionTracker(ConnectionManager *connmgr, timespec unit):
            std::thread(tmain, this, connmgr, unit), lock(), connList() {}
        inline void addConnection(Connection *conn)
        {
            lock.writeLock();
            ifsucc (conn->reference() == 0)
            {
                logDebug("Referenced by %lx", (unsigned long)this);
                logDebug("ConnectionManager::ConnectionTracker::addConnection:"
                    " Adding %lx", (unsigned long)conn);
                connList.push_back(conn);
            }
            else
            {
                logStackTraceEnd();
            }
            lock.writeRelease();
        }
    };

    std::unordered_set<
        Connection*, 
        Connection::PointerHash<>, 
        Connection::PointerEqual<>> connTable;
    RWLock lock;
    ConnectionSetHandler *handlers[Policy::Priority::count];
    ConnectionTracker *ct;
    Reflect::CreateMethod metaFactory;
public:
    struct InitObject
    {
        timespec unit;
        ConnectionSetHandler *handlers[Policy::Priority::count];
        const char *connMetaType;

        InitObject()
        {
            memset(this, 0, sizeof(InitObject));
        }
    };

    ConnectionManager(): connTable(), lock(), handlers() 
    {
        memset(handlers, 0, sizeof(handlers));
    }
    ~ConnectionManager() {}
    static void* newInstance() //@ReflectConnectionManager
    {
        return snew ConnectionManager();
    }

    int init(void *arg);

    inline int issue(DataMessage *msg)
    {
        return handlers[msg->getConnection()->getPolicy()->getPrio()]
            ->issue(msg);
    }
    
    inline void setHandler(Policy::Priority prio, ConnectionSetHandler *handler)
    {
        lock.writeLock();
        handlers[prio] = handler;
        lock.writeRelease();
    }
    int getConnection(Message *msg);
    int addConnectionNoTrack(Connection *conn);
    int addConnection(Connection *conn);
    int insertConnection(DataMessageHeader *dh, Connection **dest);
    int next(DataMessage **dest);
    inline int removeConnection(Connection *conn)
    {
        lock.writeLock();
        connTable.erase(conn);
        lock.writeRelease();
        conn->release();
        return handlers[conn->getPolicy()->getPrio()]->removeConnection(conn);
    }
    friend class ConnectionTracker;
};

}

#endif
