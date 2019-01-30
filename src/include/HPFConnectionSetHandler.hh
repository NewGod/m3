#ifndef __HPFCONNECTIONSETHANDLER_HH__
#define __HPFCONNECTIONSETHANDLER_HH__

#include <list>

#include "ConnectionSetHandler.hh"
#include "RWLock.hh"

namespace m3
{

#define HPFC_LEVEL 1
class HPFConnectionSetHandler : public ConnectionSetHandler 
{
protected:
    std::list<Connection*> connList[HPFC_LEVEL];
    RWLock lock;
public:
    HPFConnectionSetHandler():lock() {};
    int addConnection(Connection *conn) override;
    int removeConnection(Connection *conn) override;
    int next(Connection **dest) override;
    int size() override;
    void gc() override;
};

}

#endif
