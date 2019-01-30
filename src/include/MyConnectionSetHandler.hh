#ifndef __MYCONNECTIONSETHANDLER_HH__
#define __MYCONNECTIONSETHANDLER_HH__

#include <list>

#include "ConnectionSetHandler.hh"
#include "RWLock.hh"

namespace m3
{

#define HPFC_LEVEL 10
class HPFConnectionSetHandler : public ConnectionSetHandler 
{
protected:
    std::list<Connection*> connList[HPFC_LEVEL];
    RWLock lock;
	unsigned long lastPakReceived; 
	Connection* lastConn;
public:
    HPFConnectionSetHandler():lock(), lastPakReceived(), lastConn(){};
    int addConnection(Connection *conn) override;
    int removeConnection(Connection *conn) override;
    int next(Connection **dest) override;
    int size() override;
    void gc() override;
};
}

#endif
