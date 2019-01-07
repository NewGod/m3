#ifndef __MESSAGE_HH__
#define __MESSAGE_HH__

#include <netinet/ip.h>
#include <netinet/tcp.h>

#include "IPPacket.hh"
#include "Log.h"

namespace m3
{

class Connection;

class Message
{
protected:
    inline Message(Connection *connection = 0): 
        conn(connection) 
    {
        logDebug("CONMSG %lx", (unsigned long)this);
    } 
    Connection *conn;
public:
    virtual ~Message();
    virtual IPPacket* toIPPacket() = 0;
    inline Connection *getConnection()
    {
        return conn;
    }
    int setConnection(Connection *conn);
};

}

#endif
