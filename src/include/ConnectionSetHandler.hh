#ifndef __CONNECTIONSETHANDLER_HH__
#define __CONNECTIONSETHANDLER_HH__

#include "Connection.hh"
#include "DataMessage.hh"

namespace m3
{

// Each instance of this class maintains all of connections with same priority.
// Note: Implementation of this class MUST be thread-safe.
class ConnectionSetHandler
{
public:
    virtual ~ConnectionSetHandler() {}
    
    // add Connection that specified in the parameter to this object.
    virtual int addConnection(Connection *conn) = 0;

    // set `dest` to Connection that should send next packet,
    // return status of execution.
    // Note: this method will be soon replaced by 
    //   ConnectionScheduler::operator() in future version. Be ready for the
    //   migration.
    virtual int next(Connection **dest) = 0;

    // return number of Connections included in this object.
    virtual int size() = 0;

    // Provide an option to do remove dead Connections asynchronously.
    // Called periodly.
    // This method should remove a connection in `this` iff 
    // getState() ==  TCPStateMachine::CLOSED
    // class implementing removeConnection() doesn't need to implement this, 
    // use a empty function to tell the compiler about your decision.
    virtual void gc() = 0;

    // remove Connection that specified in the parameter from this object.
    // Called when a Connection is marked CLOSED.
    // class implementing gc() doesn't need to implement this, just return 1 to
    // indicate that this method didn't erased anything.
    virtual int removeConnection(Connection *conn) = 0;

    // issue a DataMessage. subclasses may override this method to do some 
    // tracking work.
    // WARNING: Caller of this method should ensure that msg->getConnection() 
    // returns valid value.
    virtual int issue(DataMessage *msg)
    {
        return msg->getConnection()->issue(msg);
    }
};

}

#endif
