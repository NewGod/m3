#ifndef __MYCONNECTIONSETHANDLER_HH__
#define __MYCONNECTIONSETHANDLER_HH__

#include <list>

#include "ConnectionSetHandler.hh"
#include "RWLock.hh"

namespace m3
{
class GreedyConnectionSetHandler : public ConnectionSetHandler 
{
protected:
    std::list<Connection*> connList;
    RWLock lock;
	unsigned long Func(Connection* t, double time);
public:
    GreedyConnectionSetHandler(): connList(), lock() {};
    int addConnection(Connection *conn) override;
    int removeConnection(Connection *conn) override;
    int next(Connection **dest) override;
    int size() override;
    void gc() override;
};
}

#endif
