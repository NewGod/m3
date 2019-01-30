#include "Connection.hh"
#include "MyConnectionSetHandler.hh"

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
int GreedyConnectionSetHandler::addConnection(Connection *conn)
{
    lock.writeLock();
    connList.push_back(conn);
    lock.writeRelease();
    conn->reference();
    return 0;
}

int GreedyConnectionSetHandler::removeConnection(Connection *conn)
{
    return 1;
}

unsigned long Func(Connection* x, double time) {
	ConnectionMetadataValue* t = (ConnectionMetadataValue*) x->getMeta()->acquireValue();
	
	if (!t) return 0;
	//ConnectionMetadataValue* t = (ConnectionMetadataValue*) x->getMeta()->getValue();
	if ( t->pakReceived > t->pakSent) logError("Error: t->pakReceived > t->pakSent");
	unsigned long ans =  t->pakReceived - t->pakSent;
	if (time - t->prev_sch_time > 10)
		ans = 0xffffffff;
	//ans = t->bytesReceived - t->bytesSent;
	x->getMeta()->releaseValue(); 
	return ans;
}

int GreedyConnectionSetHandler::next(Connection **dest)
{
	double time = getTimestamp();
	Connection *ans = NULL;
	//logErro("size: %d", size());
    lock.writeLock();
	for (auto ite = connList.begin(); ite != connList.end(); ++ite)
	{
		Connection *conn = *ite;
		if (conn->hasNext())
		{
			if (!ans || Func(conn, time) > Func(ans, time)) { 
				ans = conn;
			}
		}
	}
    lock.writeRelease();
	//logError("ans: %p", ans);
	if (ans){
		*dest = ans;
		ConnectionMetadataValue* t = (ConnectionMetadataValue*) ans->getMeta()->acquireValue();
		if (!t) return 0;
		t->prev_sch_time = time; 
		return 0;
	}
    return 1;
}

int GreedyConnectionSetHandler::size()
{
	return connList.size();
}

void GreedyConnectionSetHandler::gc()
{
    lock.writeLock();
	for (auto ite = connList.begin(); ite != connList.end();)
	{
		Connection *conn = *ite;
		iferr (conn->getState() == TCPStateMachine::CLOSED)
		{
			logMessage("HPFConnectionSetHandler::gc: "
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
