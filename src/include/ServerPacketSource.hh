#ifndef __SERVERPACKETSOURCE_HH__
#define __SERVERPACKETSOURCE_HH__

#include <map>

#include "PacketInterceptor.hh"
#include "RWLock.hh"

namespace m3
{

class DataMessage;

// we implement this class as a singleton in order to provide static 
// releasePort() call to other light-weight objects don't like to keep a 
// reference of ServerPacketSource object for themselves. 
class ServerPacketSource : public PacketInterceptor
{
protected:
    //static const int PORT_BASE = 50000;
    static ServerPacketSource *instance;

    static const int MAX_IPPACKET = 2048;
    unsigned *portMask;
    RWLock portLock;

    std::map<int, Connection*> revPortMap;

    unsigned short portBase;
    unsigned short portLen;
    unsigned short last;
    psdhdr psd;

    int allocPort(Connection *conn);
    inline bool isForwardPort(unsigned short port)
    {
        return port >= portBase && port < portBase + portLen;
    }
    Connection *getConnection(unsigned short port);
public:
    struct InitObject
    {
        PacketInterceptor::InitObject initPI;
        unsigned short portBase;
        unsigned short portLen;

        InitObject()
        {
            memset(this, 0, sizeof(InitObject));
        }
    };

    ServerPacketSource(): portLock(), revPortMap(), last(-1) {}

    void releasePort(unsigned short port);
    int forward(DataMessage *msg) override;
    int next(IPPacket *dest) override;
    int init(void *arg) override;
    static inline ServerPacketSource* getInstance()
    {
        if (instance == 0)
        {
            instance = (ServerPacketSource*)snew ServerPacketSource();
        }
        return instance;
    }
    // don't really create an instance.
    static void* newInstance() //@ReflectServerPacketSource
    {
        return getInstance();
    }
};

}

#endif
