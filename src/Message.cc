#include "Message.hh"
#include "Connection.hh"

namespace m3
{

Message::~Message()
{
    logDebug("DELMSG %lx", (unsigned long)this);
    if (conn)
    {
        conn->release();
        logDebug("Released by %lx", (unsigned long)this);
    }
}

int Message::setConnection(Connection* conn)
{
    int ret = conn->reference();
    ifsucc (ret == 0)
    {
        logDebug("Referenced by %lx", (unsigned long)this);
        this->conn = conn;
    }
    else
    {
        logError("Message::setConnection: Referencing a destroyed object.");
    }
    return ret != 0;
}

}