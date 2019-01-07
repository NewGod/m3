#include "Log.h"
#include "Session.hh"

namespace m3
{

Session::SessionTable Session::sessions;
RWLock Session::sessLock;

Session::~Session()
{
    sessLock.writeLock();
    sessions.erase(this);
    sessLock.writeRelease();
}

void Session::Metadata::hash()
{
    unsigned key[2];

    key[0] = src.sin_addr.s_addr ^ dst.sin_addr.s_addr;
    key[1] = (unsigned)dst.sin_port;

    this->hashValue = MurmurHash64A(key, sizeof(key), seed);

    logDebug("Session::Metadata::hash: %lx -> %lu", (unsigned long)this, 
        this->hashValue);
}

int Session::Metadata::init(void *arg)
{
    return ConnectionMetadataBase::init(arg);
}

// This is somehow tricky: 
// When we insert a Session to the table, we use dh->dest as server port 
// because we ASSUME client-server model is used.
// In this way, when we want to find a Session, we need to hash it for both 
// direction of the connection.
int Session::getSession(DataMessageHeader *dh, Session** dest)
{
    // init fake Session
    unsigned long sessbuf[(sizeof(Session) + 7) >> 3];
    Metadata meta;
    Session *sess = (Session*)sessbuf;
    sess->meta = &meta;
    meta.ConnectionMetadataBase::init(dh);

    sessLock.readLock();
    auto ite = sessions.find(sess);

    if (ite == sessions.end())
    {
        meta.changeDirection();

        if ((ite = sessions.find(sess)) == sessions.end())
        {
            sessLock.readRelease();

            Session *ins = snew Session(dh);
            iferr (ins == 0)
            {
                logAllocError("Session::getSession");
                return 1;
            }

            sessLock.writeLock();
            sessions.insert(ins);
            sessLock.writeRelease();
            *dest = ins;
        }
        else
        {
            sessLock.readRelease();
            *dest = *ite;
        }
    }
    else
    {
        sessLock.readRelease();
        *dest = *ite;
    }

    return 0;
}

}