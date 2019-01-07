#include "Connection.hh"
#include "RRConnectionSetHandler.hh"

namespace m3
{
	/* test */
int RRConnectionSetHandler::addConnection(Connection *conn)
{
    lock.writeLock();
    connList.push_back(conn);
    lock.writeRelease();
    conn->reference();

    return 0;
}

int RRConnectionSetHandler::removeConnection(Connection *conn)
{
    return 1;
}

int RRConnectionSetHandler::next(Connection **dest)
{
    lock.writeLock();
    for (auto ite = connList.begin(); ite != connList.end(); ++ite)
    {
        Connection *conn = *ite;
        if (conn->hasNext())
        {
            *dest = conn;
            connList.push_back(*ite);
            connList.erase(ite);
            lock.writeRelease();
            return 0;
        }
    }
    lock.writeRelease();
    return 1;
}

int RRConnectionSetHandler::size()
{
    return connList.size();
}

void RRConnectionSetHandler::gc()
{
    lock.writeLock();
    for (auto ite = connList.begin(); ite != connList.end();)
    {
        Connection *conn = *ite;
        iferr (conn->getState() == TCPStateMachine::CLOSED)
        {
            logMessage("RRConnectionSetHandler::gc: "
                "Dead connection %lx(hash %lx), remove.",
                (unsigned long)conn, conn->getHashvalue());
            ite = connList.erase(ite);
            conn->release();
            logDebug("Released by %lx", (unsigned long)this);
            continue;
        }
        ++ite;
    }
    lock.writeRelease();
}

}
