#ifndef __BLOCKRRCONNECTIONSETHANDLER_HH__
#define __BLOCKRRCONNECTIONSETHANDLER_HH__

#include "RRConnectionSetHandler.hh"
#include "Semaphore.hh"

namespace m3
{

class BlockRRConnectionSetHandler : public RRConnectionSetHandler
{
protected:
    Semaphore cnt; 
public:
    BlockRRConnectionSetHandler(): cnt(0) {}
    int issue(DataMessage *msg);
    int next(Connection **dest);
};

}

#endif
