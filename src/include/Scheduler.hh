#ifndef __SCHEDULER_HH__
#define __SCHEDULER_HH__

namespace m3
{

class Connection;
class ContextManager;
class Interface;

class Scheduler
{
public:
    virtual ~Scheduler() {}
    int init(ContextManager *cm, Connection *conn)
    {
        this->cm = cm;
        this->conn = conn;
        return 0;
    }
    // returns index of the interface in specified connection
    virtual int schedule(Interface **dest, Message *msg) = 0;
protected:
    ContextManager *cm;
    Connection *conn;
};

}

#endif
