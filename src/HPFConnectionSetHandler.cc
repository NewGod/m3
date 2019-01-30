#include "Connection.hh"
#include "HPFConnectionSetHandler.hh"

namespace m3
{

int HPFConnectionSetHandler::addConnection(Connection *conn)
{
    lock.writeLock();
    connList[0].push_back(conn);
    lock.writeRelease();
    conn->reference();

    return 0;
}

int HPFConnectionSetHandler::removeConnection(Connection *conn)
{
    return 1;
}

int HPFConnectionSetHandler::next(Connection **dest)
{
    lock.writeLock();
	for (int i = 0; i < HPFC_LEVEL; i++){ 
		for (auto ite = connList[i].begin(); ite != connList[i].end(); ++ite)
		{
			Connection *conn = *ite;
			if (conn->hasNext())
			{
				*dest = conn;
				if (i + 1 < HPFC_LEVEL) {
					connList[i + 1].push_back(*ite);
					connList[i].erase(ite);
				}
				lock.writeRelease();
				return 0;
			}
		}
	}
    lock.writeRelease();
    return 1;
}

int HPFConnectionSetHandler::size()
{
	int sum = 0;
	for (int i = 0; i < HPFC_LEVEL; i++) 
		sum += connList[i].size();
	return sum;
}

void HPFConnectionSetHandler::gc()
{
    lock.writeLock();
	for (int i = 0; i < HPFC_LEVEL; i++) {
		for (auto ite = connList[i].begin(); ite != connList[i].end();)
		{
			Connection *conn = *ite;
			iferr (conn->getState() == TCPStateMachine::CLOSED)
			{
				logMessage("HPFConnectionSetHandler::gc: "
					"Dead connection %lx(hash %lx), remove.",
					(unsigned long)conn, conn->getHashvalue());
				ite = connList[i].erase(ite);
				conn->release();
				logDebug("Released by %lx", (unsigned long)this);
				continue;
			}
			++ite;
		}
	}
    lock.writeRelease();
}
}
