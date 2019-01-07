#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "NaiveTCPMessageProvider.hh"
#include "ServerInterfaceManager.hh"

namespace m3
{

int ServerInterfaceManager::initInterfaces(ConfigArray *intf, 
    ConnectionClassifier *clasf, PacketSource *source)
{
    ListenInf *listen;
    int ret;
    int imss;
    int imtu;
    int llen = 0, lp = 0;

    for (int i = 0; i < intf->len; ++i)
    {
        int count = getConfigField(count, intf->arr[i].object, Int);
        llen += count < 1 ? 1 : count;
    }

    if ((listen = snew ListenInf[llen]) == 0)
    {
        logAllocError("ServerInterfaceManager::initInterfaces");
        return 1;
    }

    for (int i = 0; i < intf->len; ++i)
    {
        ConfigObject *obj;
        const char* device;
        const char* src;
        unsigned short srcp;
        int count;
        int fd;

        iferr (intf->arr[i].type != ConfigField::Type::OBJECT)
        {
            logError("ServerInterfaceManager::initInterfaces: "
                "m3-ListenAddress[%d] is not a json object.", i);
            delete[] listen;
            return 2;
        }
        iferr (!intf->arr[i].object->isClass("ListenAddressConfig"))
        {
            logError("ServerInterfaceManager::initInterfaces: "
                "m3-ListenAddress[%d] "
                "type mismatch(%s), expected ListenAddressConfig.",
                i, intf->arr[i].object->getFieldClassName());
            delete[] listen;
            return 3;
        }
        obj = intf->arr[i].object;
        device = getConfigField(device, obj, String);
        src = getConfigField(src, obj, String);
        srcp = getConfigField(srcp, obj, Int);
        count = getConfigField(count, obj, Int);
        count = count < 1 ? 1 : count;

        iferr ((fd = open_listenfd(src, srcp)) < 0)
        {
            char errbuf[64];
            logError("ServerInterfaceManager::initInterfaces: Cannot listen" 
                " on %s:%d(%s)", src, srcp, strerrorV(errno, errbuf));
            delete[] listen;
            return 4;
        }

        logMessage("ServerInterfaceManager::initInterfaces: Listening" 
            " on %s:%d", src, srcp);
        
        while (count-- > 0)
        {
            listen[lp].dev = device;
            listen[lp].ip = src;
            listen[lp].port = srcp;
            listen[lp].listenfd = fd;
            listen[lp].connfd = -1;
            lp++;
        }
    }

    logMessage("ServerInterfaceManager::initInterfaces: Listening...");

    for (int i = 0; i < intf->len;)
    {
        fd_set fdset;
        int ret;
        int maxfd = -1;
        FD_ZERO(&fdset);

        for (int j = 0; j < intf->len; ++j)
        {
            if (listen[j].connfd == -1)
            {
                FD_SET(listen[j].listenfd, &fdset);
                maxfd = maxfd > listen[j].listenfd ? 
                    maxfd : listen[j].listenfd;
            }
        }

        iferr ((ret = select(maxfd + 1, &fdset, NULL, NULL, NULL)) < 0)
        {
            char errbuf[64];
            logError("ServerInterfaceManager::initInterfaces: "
                "select failed(%s)", strerrorV(errno, errbuf));
            delete[] listen;
            return 5;
        }

        for (int j = 0; j < intf->len; ++j)
        {
            sockaddr_in clientaddr;
            unsigned int clientlen = sizeof(sockaddr_in);
            if (listen[j].connfd == -1 &&
                FD_ISSET(listen[j].listenfd, &fdset))
            {
                iferr ((listen[j].connfd = accept(listen[j].listenfd, 
                    (sockaddr*)&clientaddr, &clientlen)) < 0)
                {
                    char errbuf[64];
                    logError("ServerInterfaceManager::initInterfaces: "
                        "accept failed for interface %d(%s).", j,
                        strerrorV(errno, errbuf));
                    delete[] listen;
                    return 6;
                }
                logMessage("ServerInterfaceManager::initInterfaces: "
                    "Connected with %s:%d", inet_ntoa(clientaddr.sin_addr), 
                    ntohs(clientaddr.sin_port));
                ++i;
            }
        }
    }

    for (int i = 0; i < llen; ++i)
    {
        if (!(i && listen[i].listenfd == listen[i - 1].listenfd))
        {
            close(listen[i].listenfd);
        }
    }
    
    minmss = 0;
    maxmtu = 0;
    for (int i = 0; i < intf->len; ++i)
    {
        Interface::InitObject iarg;
        Interface *interface;
        iferr ((interface = snew Interface()) == 0)
        {
            logAllocError("ServerInterfaceManager::initInterfaces");
            logError("In m3-ListenAddress[%d].", i);
            return 7;
        }

        iarg.device = listen[i].dev;
        iarg.fsname = "RobustFileStream";
        iarg.initFM.fd = listen[i].connfd;
        iarg.initFM.connmgr = connmgr;
        iarg.initFM.clasf = clasf;
        iarg.initFM.source = source;

        iferr ((ret = createTCPMessageProvider(&iarg.initFM.kmp,
            iarg.device, iarg.initFM.fd)) != 0)
        {
            logStackTrace("ServerInterfaceManager::initInterfaces", ret);
            logError("In m3-FlowAddrPairs[%d].", i);
            return 8;
        }

        iarg.provider = iarg.initFM.kmp->getProvider();

        iferr ((ret = interface->init(&iarg)) != 0)
        {
            logStackTrace("ServerInterfaceManager::initInterfaces", ret);
            logError("In m3-ListenAddress[%d].", i);
            return 9;
        }

        imss = interface->getMSS();
        imtu = interface->getMTU();

        minmss = minmss == 0 ? imss : (minmss < imss ? minmss : imss);
        maxmtu = maxmtu == 0 ? imtu : (maxmtu > imtu ? maxmtu : imtu);

        addInterface(interface);
    }

    minmss -= DataMessageHeader::MSS_REDUCE_VAL;

    return 0;
}

}