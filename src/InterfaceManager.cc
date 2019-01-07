#include <time.h>

#include "DataMessage.hh"
#include "NetworkUtils.h"
#include "InterfaceManager.hh"
#include "LivePCAP.hh"
#include "NaiveTCPMessageProvider.hh"
#include "PCAPInitializer.hh"
#include "Represent.h"
#include "ThreadUtils.hh"

namespace m3
{

Interface** InterfaceManager::getAllInterfaces(int *len)
{
    *len = interfaces.size();
    return interfaces.data();
}

void InterfaceManager::addInterface(Interface *interface)
{
    interfaces.push_back(interface);
}

// todo: move sockaddr initialization code into TCPMessageProvider
// (logically belong to that class)
int InterfaceManager::createTCPMessageProvider(KernelMessageProvider **dest,
    const char *name, int fd)
{
    NaiveTCPMessageProvider *tmp;
    NaiveTCPMessageProvider::InitObject iobj;
    int snaplen = 100;
    socklen_t socklen = sizeof(int);
    int mss;
    int ret;

    iferr (getsockopt(fd, IPPROTO_TCP, 2, &mss, &socklen) < 0)
    {
        char errbuf[64];
        logError("InterfaceManager::createTCPMessageProvider: "
            "Cannot get MSS.(%s)", strerrorV(errno, errbuf));
        return 1;
    }

    iobj.handletype = "PCAPInitializer";
    iobj.device = name;
    iobj.handlepriv = (void*)&snaplen;
    socklen = sizeof(sockaddr_in);
    iferr (getsockname(fd, (sockaddr*)&iobj.myaddr, 
        &socklen) < 0)
    {
        char errbuf[64];
        logError("InterfaceManager::createTCPMessageProvider: "
            "Can't get socket address(%s).", strerrorV(errno, errbuf));
        return 4;
    }
    socklen = sizeof(sockaddr_in);
    iferr (getpeername(fd, (sockaddr*)&iobj.peeraddr, 
        &socklen) < 0)
    {
        char errbuf[64];
        logError("InterfaceManager::createTCPMessageProvider:" 
            "Can't get peer address(%s).", strerrorV(errno, errbuf));
        return 5;
    }

    iferr ((tmp = snew NaiveTCPMessageProvider()) == 0)
    {
        logAllocError("InterfaceManager::createTCPMessageProvider");
        return 6;
    }

    iobj.fd = fd;
    iferr ((ret = tmp->init(&iobj)) != 0)
    {
        logStackTrace("InterfaceManager::createTCPMessageProvider", ret);
        return 7;
    }
    ((NaiveTCPStreamWatcher*)tmp->getProvider())->setMSS(mss);

    *dest = tmp;

    return 0;
}

int InterfaceManager::initInterfaces(ConfigArray *pairs, ConnectionClassifier 
    *clasf, PacketSource *source)
{
    minmss = 0;
    maxmtu = 0;
    for (int i = 0; i < pairs->len; ++i)
    {
        Interface *interface;
        ConfigObject *obj;
        int ret;
        const char* src;
        const char* dst;
        unsigned short srcp;
        unsigned short dstp;
        int imss;
        int imtu;
        Interface::InitObject iarg;

        iferr (pairs->arr[i].type != ConfigField::Type::OBJECT)
        {
            logError("InterfaceManager::initInterfaces: m3-FlowAddrPairs[%d] "
                "is not a json object.", i);
            return 1;
        }
        iferr (!pairs->arr[i].object->isClass("FlowAddressPairConfig"))
        {
            logError("InterfaceManager::initInterfaces: m3-FlowAddrPairs[%d] "
                "type mismatch(%s), expected FlowAddressPairConfig.",
                i, pairs->arr[i].object->getFieldClassName());
            return 2;
        }
        iferr ((interface = snew Interface()) == 0)
        {
            logAllocError("InterfaceManager::initInterfaces");
            logError("In m3-FlowAddrPairs[%d].", i);
            return 3;
        }
        obj = pairs->arr[i].object;
        iarg.device = getConfigField(device, obj, String);
        src = getConfigField(src, obj, String);
        dst = getConfigField(dst, obj, String);
        srcp = getConfigField(srcp, obj, Int);
        dstp = getConfigField(dstp, obj, Int);

        iferr ((iarg.initFM.fd = netdial(
            AF_INET, SOCK_STREAM, src, srcp, dst, dstp)) < 0)
        {
            char errbuf[64];
            logError("InterfaceManager::initInterfaces: Cannot establish" 
                "connection using addr %s:%d<->%s:%d(%s)", src, srcp, dst, dstp,
                strerrorV(errno, errbuf));
            logError("In m3-FlowAddrPairs[%d].", i);
            return 4;
        }

        iferr ((ret = createTCPMessageProvider(&iarg.initFM.kmp,
            iarg.device, iarg.initFM.fd)) != 0)
        {
            logStackTrace("InterfaceManager::initInterfaces", ret);
            logError("In m3-FlowAddrPairs[%d].", i);
            return 5;
        }

        iarg.initFM.connmgr = connmgr;
        iarg.initFM.clasf = clasf;
        iarg.initFM.source = source;
        iarg.fsname = "RobustFileStream";
        iarg.provider = iarg.initFM.kmp->getProvider();
        iferr ((ret = interface->init(&iarg)) != 0)
        {
            logStackTrace("InterfaceManager::initInterfaces", ret);
            logError("In m3-FlowAddrPairs[%d].", i);
            return 6;
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

int InterfaceManager::init(ConnectionManager *connmgr, ConfigArray *intf,
    ConnectionClassifier *clasf, PacketSource *source)
{
    int ret;
    this->connmgr = connmgr;

    iferr ((ret = initInterfaces(intf, clasf, source)) != 0)
    {
        logStackTrace("InterfaceManager::init", ret);
        return 1;
    }

    iferr ((sw = snew SendWorker(
        this, minmss, interfaces.size(), connmgr)) == 0)
    {
        logAllocError("InterfaceManager::init");
        return 2;
    }
    return 0;
}

void InterfaceManager::SendWorker::tmain(InterfaceManager *me,
    int minmss, int size, ConnectionManager *connmgr)
{
    Interface **arr;
    DataMessage *msg = 0;
    std::map<DataMessage*, int> pendingmap;

    logMessage("InterfaceManager::SendWorker::tmain: Thread identify");

#ifdef ENABLE_FORCE_THREAD_SCHEDULE
    int enabled = (SetRealtime() <= 0);
    AssignCPU();
    if(enabled)
    {
        logMessage("InterfaceManager::SendWorker::tmain: Thread set to real-time");
    }
    else
    {
        logError("InterfaceManager::SendWorker::tmain: Failed to set to real-time");
    }
#endif

    if ((arr = snew Interface*[size]) == 0)
    {
        // todo: implement a wrapper of thread main function to enable full 
        // logging support(with return value)
        logAllocError("InterfaceManager::SendWorker::tmain");
        return;
    }
    Interface **all = me->interfaces.data();
    int len = me->interfaces.size();
    int reschedule = 0;
    while (1)
    {
        int ret;
        int sendcnt;
        DataMessageHeader *dh;
        if (!reschedule)
        {
            logDebug("InterfaceManager::SendWorker::tmain: Getting message");
            while (connmgr->next(&msg) != 0);
            logDebug("InterfaceManager::SendWorker::tmain: Got message");
        }
        dh = msg->getHeader();
        //if (dh->syn == 1)
        //{
        //    logMessage("SYN");
        //    PacketReader::m3Decode(dh);
        //}
        sendcnt = msg->getConnection()->schedule(arr, msg);
        iferr (sendcnt == 0)
        {
            //logStackTrace("InterfaceManager::SendWorker::tmain", sendcnt);
            //logStackTraceEnd();
            reschedule = 1;

            int alen;
            Interface **avail = msg->getConnection()->getPolicy()->
                acquireInterfaces(&alen);
            bool noAvail = true;
            while (noAvail)
            {
                for (int i = 0; i < alen; ++i)
                {
                    DataMessage *msg = avail[i]->sendPending();
                    if (msg != 0)
                    {
                        if ((pendingmap[msg] = pendingmap[msg] - 1) == 0)
                        {
                            pendingmap.erase(msg);
                            delete msg;
                        }
                        noAvail = false;
                    }
                }
            }
            logVerbose("InterfaceManager::SendWorker::tmain: Reschedule");
            continue;
        }

        if (unlikely(dh->syn))
        {
            if (dh->modifyMSS(minmss) != 0)
            {
                IPPacket *pak = msg->toIPPacket();
                PacketReader::packetDecode(pak->getIPHeader(), pak->getIPLen());
                PacketReader::m3Decode((DataMessageHeader*)pak->dataHeader);
            }
        }

        for (int i = 0; i < len; ++i)
        {
            DataMessage *msg = all[i]->sendPending();
            if (msg != 0)
            {
                if ((pendingmap[msg] = pendingmap[msg] - 1) == 0)
                {
                    pendingmap.erase(msg);
                    delete msg;
                }
            }
        }

        bool renewDone = false;
        int nr = 0;
        for (int i = 0; i < sendcnt; ++i)
        {
            iferr ((ret = arr[i]->send(msg)) < 0)
            {
                logStackTrace("InterfaceManager::SendWorker::tmain", ret);
                logStackTraceEnd();
            }
            else if (!arr[i]->isReady())
            {
                nr++;
            }
            else if (!renewDone)
            {
                renewDone = true;
                msg->getConnection()->renewOnOutput(msg);
            }
        }
        if (nr == 0)
        {
            delete msg;
        }
        else
        {
            pendingmap[msg] = nr;
        }
        reschedule = 0;

// #ifdef ENABLE_FORCE_THREAD_SCHEDULE
//         timespec req = {0, 1000};
//         nanosleep(&req, 0);
// #endif
    }
}

}