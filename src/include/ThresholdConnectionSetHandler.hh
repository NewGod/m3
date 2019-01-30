#ifndef __THRESHOLDCONNECTIONSETHANDLER_HH__
#define __THRESHOLDCONNECTIONSETHANDLER_HH__

#include <list>

#include "ConnectionSetHandler.hh"
#include "RWLock.hh"

namespace m3{
class ThresholdConnectionSetHandler : public ConnectionSetHandler 
{
protected:
    std::list<Connection*> connList;
	std::list<Connection*> scheList;
    RWLock lock;
	unsigned long threshold;
	unsigned long Func(Connection* t, double time);
public:
    ThresholdConnectionSetHandler(): connList(), lock(), threshold(0){};
    int addConnection(Connection *conn) override;
    int removeConnection(Connection *conn) override;
    int next(Connection **dest) override;
    int size() override;
    void gc() override;
};
}
#endif
