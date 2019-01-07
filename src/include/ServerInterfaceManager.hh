#ifndef __SERVERINTERFACEMANAGER_HH__
#define __SERVERINTERFACEMANAGER_HH__

#include "InterfaceManager.hh"

namespace m3
{

class ServerInterfaceManager : public InterfaceManager
{
public:
    int initInterfaces(ConfigArray *pairs, ConnectionClassifier *clasf, 
        PacketSource *source) override;
    static void* newInstance() //@ReflectServerInterfaceManager
    {
        return snew ServerInterfaceManager();
    }
protected:
    struct ListenInf
    {
        const char *dev;
        const char *ip;
        unsigned short port;
        int listenfd;
        int connfd;
    };
};

}

#endif
