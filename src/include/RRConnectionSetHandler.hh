#ifndef __RRCONNECTIONSETHANDLER_HH__
#define __RRCONNECTIONSETHANDLER_HH__

#include <list>

#include "ConnectionSetHandler.hh"
#include "RWLock.hh"

namespace m3
{

class RRConnectionSetHandler : public ConnectionSetHandler 
{
protected:
    std::list<Connection*> connList;
    RWLock lock;
public:
    RRConnectionSetHandler(): connList(), lock() {}
    int addConnection(Connection *conn) override;
    int removeConnection(Connection *conn) override;
    int next(Connection **dest) override;
    int size() override;
    void gc() override;
};

}

#endif
