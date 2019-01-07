#ifndef __INTERFACE_HH__
#define __INTERFACE_HH__

#include <sys/types.h>

#include <set>

#include "DataMessage.hh"
#include "ForwardModule.hh"
#include "RobustFileStream.hh"
#include "ProtocolMetric.hh"
#include "AsynchronousTCPContext.hh"

namespace m3
{

class ConnectionClassifier;
class ConnectionManager;
class MetricProvider;
class KernelMessage;
class Message;
class PacketSource;

class Interface
{
public:
    struct InitObject
    {
        ForwardModule::InitObject initFM;
        const char *fsname;
        const char *device;
        MetricProvider *provider;

        InitObject()
        {
            memset(this, 0, sizeof(InitObject));
        }
    };

    static const int MAX_METRIC_COUNT = 8;
    static const int NAME_LEN = 15;
    const char name[NAME_LEN + 1];
    // pcap_lookupnet
    const unsigned ip;
    const unsigned mask;
    int fd;
    static AsynchronousTCPContext *aTCPCxt;
    
    Interface();
    virtual ~Interface();

    virtual int init(void *arg);
    //int init(const char *device, int fd, ConnectionManager *connmgr, 
    //    ConnectionClassifier *clasf, PacketSource *source);
    
    bool isReady();
    //int restart();
    // TODO: override send() to implement dump output
    // make ForwardModule virtual to implement file input
    virtual int send(DataMessage *msg);

    inline int getMSS()
    {
        return mss;
    }
    inline int getMTU()
    {
        return mtu;
    }
    inline MetricProvider *getProvider()
    {
        return mp;
    }
    inline DataMessage* sendPending()
    {
        DataMessage *res = 0;
        if (pending.load() != 0)
        {
            pending.load()->write(fs);
            if (pending.load()->written())
            {
                res = pending;
                pending = 0;
            }
        }
        return res;
    }

    static void* newInstance() //@ReflectInterface
    {
        return snew Interface();
    }
protected:
    int setMTU();

    FileStream *fs;
    MetricProvider *mp;
    ForwardModule *fm;
    std::atomic<DataMessage*> pending;
    int pos;

    int mss;
    int mtu;

    friend class ForwardModule;
};

}

#endif
