#ifndef __CONNECTION_HH__
#define __CONNECTION_HH__

#include <sys/types.h>

#include <atomic>
#include <cstring>

#include "AutoDestroyable.hh"
#include "BlockQueueBuffer.hh"
#include "ConnectionMetadata.hh"
#include "ConnectionType.hh"
#include "Log.h"
#include "Policy.hh"
#include "SpinQueueBuffer.hh"
#include "TCPStateMachine.hh"

namespace m3
{

class DataMessage;
class DataMessageHeader;
class IPPacket;
class Interface;
class Message;

class Connection : public AutoDestroyable 
{
protected:
    ConnectionMetadataBase *meta;

    // variables offering thread-safe methods
    Policy policy;
    std::atomic<ConnectionType*> type;
    SpinPointerQueueBuffer<DataMessage> outbuf;
    SpinPointerQueueBuffer<DataMessage> retxbuf;
    TCPStateMachine state;
public:
    typedef TCPStateMachine::State State;

    virtual ~Connection();

    inline Connection(DataMessageHeader *dh,
        ConnectionMetadataBase *meta = 0):  meta(meta), policy(), 
        type(0), outbuf(), retxbuf(), state()
    {
        if (this->meta == 0)
        {
            this->meta = new ConnectionMetadataBase();
        }
        iferr (this->meta->init(dh) != 0)
        {
            throw 1;
        }
    }

    inline ConnectionMetadataBase *getMeta()
    {
        return meta;
    }
    inline unsigned long getHashvalue()
    {
        return meta->hashValue;
    }
    inline ConnectionType* getType()
    {
        return type.load();
    }
    inline void setType(ConnectionType* type)
    {
        ConnectionType *oldType = this->type.exchange(type);
        if (oldType != 0 && oldType != type)
        {
            oldType->release();
        }
        type->reference();
    }
    inline sockaddr_in *getDest()
    {
        return &meta->dst;
    }
    inline sockaddr_in *getSource()
    {
        return &meta->src;
    }
    inline unsigned short getProxyPort()
    {
        return meta->proxyPort;
    }
    inline void setProxyPort(unsigned short port)
    {
        meta->proxyPort = port;
    }
    inline int getMSS()
    {
        return meta->mss;
    }
    inline void setMSS(int mss)
    {
        meta->mss = mss;
    }
    inline Policy* getPolicy()
    {
        return &policy;
    }
    inline State getState()
    {
        return state.getState();
    }
    inline TCPStateMachine *getStateMachine()
    {
        return &state;
    }

    inline bool operator==(const Connection &rv) const
    {
        return *meta == *rv.meta;
    }

    inline int issue(DataMessage *msg)
    {
        iferr (outbuf.tryAdvance(&msg) != 0)
        {
            logError("Connection::issue: No buffer space available.");
            return 1;
        }
        //logMessage("ADV %lu", (unsigned long)msg);
        return 0;
    }
    
    int schedule(Interface **ret, Message *msg);
    virtual bool hasNext();
    virtual int next(DataMessage **ret);
    // maintain TCP state machine & meta data
    virtual int renewOnOutput(Message *message);
    virtual int renewOnInput(Message *message);

    template <typename T = Connection>
    class PointerHash
    {
    public:
        unsigned long operator()(T* const& px) const
        {
            return px->meta->hashValue;
        }
    };

    template <typename T = Connection>
    class PointerEqual
    {
    public:
        bool operator()(T* const& pl, T* const& pr) const
        {
            return *pl == *pr;
        }
    };

    friend class ConnectionManager;
};

class StatelessConnection : public Connection
{
public:
    inline StatelessConnection(DataMessageHeader *dh): Connection(dh) {}
    int renewOnInput(Message *msg) override;
    int renewOnOutput(Message *msg) override;
};

}

#endif
