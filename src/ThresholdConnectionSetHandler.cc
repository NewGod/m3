#include "Connection.hh"
#include "ThresholdConnectionSetHandler.hh"
namespace m3
{

int ThresholdConnectionSetHandler::addConnection(Connection *conn)
{
    lock.writeLock();
    connList.push_back(conn);
    lock.writeRelease();
    conn->reference();
    return 0;
}

int ThresholdConnectionSetHandler::removeConnection(Connection *conn)
{
    return 1;
}

unsigned long ThresholdConnectionSetHandler::Func(Connection* x, double time) {
	ConnectionMetadataValue* t = (ConnectionMetadataValue*) x->getMeta()->acquireValue();
	if (!t) {
		x->getMeta()->releaseValue(); 
		return 0;
	}
	//ConnectionMetadataValue* t = (ConnectionMetadataValue*) x->getMeta()->getValue();
	if ( t->pakReceived > t->pakSent) logError("Error: t->pakReceived > t->pakSent");
	unsigned long ans =  t->pakReceived - t->pakSent;
	if (time - t->prev_sch_time > 10)
		ans = 0xffffffff;
	//ans = t->bytesReceived - t->bytesSent;
	x->getMeta()->releaseValue(); 
	return ans;
}

int ThresholdConnectionSetHandler::next(Connection **dest)
{
	double time = getTimestamp();
	//logErro("size: %d", size());
    lock.writeLock();
	for (auto ite = scheList.begin(); ite != scheList.end();)
	{
		Connection *conn = *ite;
		++ite;
		scheList.pop_front();
		if (conn->hasNext()) {
			lock.writeRelease();
			*dest = conn;
			ConnectionMetadataValue* t = (ConnectionMetadataValue*) conn->getMeta()->getValue();
			if (!t) {
				return 0;
			}
			t->prev_sch_time = time; 
			return 0;
		}
	}
	// update;
	for (auto ite = connList.begin(); ite != connList.end(); ++ite) { 
		Connection *conn = *ite;
		if (conn->hasNext() && Func(conn, time) >= threshold) 
			scheList.push_back(conn);
	}
	// Threshold update;
#define MX_LIMIT 20
#define MN_LIMIT 5
#define MX_THRESHOLD 1024
#define MN_THRESHOLD 0
	if (scheList.size() > MX_LIMIT) {
		if (threshold < MX_THRESHOLD) {
			if (threshold) threshold *= 2;
			else threshold = 1;
		}
	}else if (scheList.size() < MN_LIMIT) {
		if (threshold > MN_THRESHOLD) threshold /= 2;
	}
	lock.writeRelease();
	return 1;
}

int ThresholdConnectionSetHandler::size()
{
	return connList.size();
}

void ThresholdConnectionSetHandler::gc()
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
