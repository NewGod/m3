#ifndef __CONNECTIONMETADATA_HH__
#define __CONNECTIONMETADATA_HH__

#include <sys/socket.h>
#include <netinet/in.h>

#include "ConcurrentValueHolder.hh"
#include "HashObject.hh"
#include "Represent.h"

namespace m3
{

class DataMessageHeader;
class Message;
class Session;

// ConnectionMetadata is a class designed to be an alternation of 
// deprecated ConnectionType. In the future ConnectionType will be a part
// of ConnectionMetadata, and this class would hold all values that users
// would like to attach to a Connection. 
// Note: use acquireValue() & releaseValue() if you need to read protected
// values in ConnectionMetadataValue 
class ConnectionMetadataBase : public ConcurrentValueHolder, 
    public HashObject
{
protected:
    static long seed;
    
    // const value, initialized on first renew(). no need to protect.
    int mss;
    unsigned short proxyPort;
    sockaddr_in src;
    sockaddr_in dst;
public:
    virtual ~ConnectionMetadataBase() {}
    ConnectionMetadataBase(): ConcurrentValueHolder(), HashObject(),
        proxyPort(0) {}

    inline bool operator==(const ConnectionMetadataBase &rv) const
    {
        long lsp = ((long)src.sin_port << 32) | src.sin_addr.s_addr;
        long rsp = ((long)rv.src.sin_port << 32) | rv.src.sin_addr.s_addr;
        long ldp = ((long)dst.sin_port << 32) | dst.sin_addr.s_addr;
        long rdp = ((long)rv.dst.sin_port << 32) | rv.dst.sin_addr.s_addr;

        //logDebug("comp %lx %lx %lx %lx", lsp, ldp, rsp, rdp);

        return (lsp == rsp && ldp == rdp) || (lsp == rdp && ldp == rsp);
    }
    virtual int init(void *arg);
    virtual void renewOnReorderSchedule(Message *msg)
    {
        return;
    }
    virtual void renewOnOutput(Message *msg)
    {
        return;
    }
    virtual void renewOnInput(Message *msg)
    {
        return;
    }
    virtual const void* getValue() override
    {
        return 0;
    }
    virtual void hash() override;

    static void* newInstance() //@ReflectConnectionMetadataBase
    {
        return snew ConnectionMetadataBase();
    }
    
    friend class Connection;
};

struct ConnectionMetadataValue
{
    //virtual void dummy(){}//used for polymorphic
public:
    // const value, initialized on first renew(). no need to protect.
    bool timeValid;
    timespec startTime;
    Session *session;

    // Protected value, acquire lock before reading
    unsigned long bytesSent;
    unsigned long bytesReceived;
    unsigned long pakSent;
    unsigned long pakReceived;

    ConnectionMetadataValue(): timeValid(false), startTime({0, 0}), 
        session(0), bytesSent(0), bytesReceived(0), pakSent(0), 
        pakReceived(0) {}
};

class ConnectionMetadata : protected ConnectionMetadataValue,
    public ConnectionMetadataBase
{
public:
    ConnectionMetadata(): ConnectionMetadataValue(),
        ConnectionMetadataBase() {}
    virtual ~ConnectionMetadata();

    virtual void renewOnInput(Message *msg) override;
    virtual void renewOnOutput(Message *msg) override;
    virtual const void* getValue() override
    {
        return (ConnectionMetadataValue*)this;
    }
    virtual int init(void *arg) override;

    static void* newInstance() //@ReflectConnectionMetadata
    {
        return snew ConnectionMetadata();
    }
};

}

#endif
