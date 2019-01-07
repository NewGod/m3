#ifndef __INTERFACEMANAGER_HH__
#define __INTERFACEMANAGER_HH__

#include <thread>
#include <vector>

#include "Config.hh"
#include "ConnectionManager.hh"
#include "Interface.hh"

namespace m3
{

class ConnectionClassifier;
class KernelMessageProvider;
class PacketSource;

// Do sending work here
class InterfaceManager
{
protected:
    class SendWorker : public std::thread
    {
    protected:
        static void tmain(InterfaceManager *me, int minmss, int size,
            ConnectionManager *psource);
    public:
        inline SendWorker(InterfaceManager *me, int minmss, 
            int size, ConnectionManager *psource) : 
            thread(tmain, me, minmss, size, psource) {}
    };

    int createTCPMessageProvider(KernelMessageProvider **dest,
        const char *name, int fd);
    void addInterface(Interface* interface);
    virtual int initInterfaces(ConfigArray *intf, ConnectionClassifier *clasf, 
        PacketSource *source);
    SendWorker *sw;
    std::vector<Interface*> interfaces;
    ConnectionManager *connmgr;
    int maxmtu;
    int minmss;
public:
    InterfaceManager(): interfaces() {}
    virtual ~InterfaceManager() {}
    int init(ConnectionManager *connmgr, ConfigArray *interfaces,
        ConnectionClassifier *clasf, PacketSource *source);
    Interface** getAllInterfaces(int *len);
    static void* newInstance() //@ReflectInterfaceManager
    {
        return snew InterfaceManager();
    }

    inline int getMinMSS()
    {
        return minmss;
    }

    inline int getMaxMTU()
    {
        return maxmtu;
    }
};

}

#endif
