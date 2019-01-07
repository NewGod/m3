#ifndef __SESSION_HH__
#define __SESSION_HH__

#include <unordered_set>

#include "Connection.hh"
#include "ConnectionMetadata.hh"
#include "RWLock.hh"

namespace m3
{


class Session : public Connection
{
public:
    class Metadata : public ConnectionMetadata
    {
    public:
        Metadata(): ConnectionMetadata() {}
        int init(void *arg) override;
        void hash() override;
        // Swap ports only. Not _really_ changes the direction. 
        inline void changeDirection()
        {
            dst.sin_port ^= src.sin_port;
            src.sin_port ^= dst.sin_port;
            dst.sin_port ^= src.sin_port;
        }
    };
    using ConnectionTable = std::unordered_set<
        Connection*, 
        Connection::PointerHash<>, 
        Connection::PointerEqual<>>;
    using SessionTable = std::unordered_set<
        Session*, 
        Connection::PointerHash<Session>, 
        Connection::PointerEqual<Session>>;
protected: 
    static SessionTable sessions;
    static RWLock sessLock;
    ConnectionTable connTable;
    RWLock connTableLock;
public:
    Session(DataMessageHeader *dh): Connection(dh, snew Metadata()),
        connTable(), connTableLock() {}
    Session(DataMessageHeader *dh, Metadata *meta): Connection(dh, meta),
        connTable(), connTableLock() {}
    ~Session();

    static int getSession(DataMessageHeader *dh, Session **dest);
    inline void removeConnection(Connection *conn)
    {
        connTableLock.writeLock();
        connTable.erase(conn);
        connTableLock.writeRelease();
    }
    inline void addConnection(Connection *conn)
    {
        connTableLock.writeLock();
        connTable.insert(conn);
        connTableLock.writeRelease();
    }
    inline const ConnectionTable* acquireConnTable()
    {
        connTableLock.readLock();
        return &connTable;
    }
    inline void releaseConnTable()
    {
        connTableLock.readRelease();
    }
};

}

#endif
