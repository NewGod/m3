#include "NaiveTCPMessageProvider.hh"
#include "PacketReader.hh"
#include "TCPBasicMetric.hh"

namespace m3
{

int NaiveTCPMessageProvider::init(void *arg)
{
    InitObject *iarg = (InitObject*)arg;
    NaiveTCPStreamWatcher::InitObject tswInit;
    ProtocolMetric *metrics[1];
    int ret;

    iferr ((mp = snew NaiveTCPStreamWatcher()) == 0 ||
        (*metrics = snew TCPBasicMetric()) == 0)
    {
        logAllocError("NaiveTCPMessageProvider::init");
        return 1;
    }
        
    ((TCPBasicMetric*)*metrics)->setIP(iarg->myaddr.sin_addr.s_addr);

    tswInit.myaddr = iarg->myaddr;
    tswInit.peeraddr = iarg->peeraddr;
    tswInit.prInit.handletype = iarg->handletype;
    tswInit.prInit.handlepriv = iarg->handlepriv;
    tswInit.prInit.device = iarg->device;
    tswInit.prInit.headerOffset = sizeof(ethhdr);
    tswInit.metrics = metrics;
    tswInit.metricCount = 1;
    tswInit.fd = iarg->fd;
    iferr ((ret = ((NaiveTCPStreamWatcher*)mp)->init(&tswInit)) != 0)
    {
        logStackTrace("NaiveTCPMessageProvider::init", ret);
        return 2;
    }

    return 0;
}

int NaiveTCPMessageProvider::next(KernelMessage **dest)
{
    KernelMessage *res;
    int ret;
    IPPacket pak;
    NaiveTCPStreamWatcher *tw = (NaiveTCPStreamWatcher*)mp;

    while (unlikely((ret = tw->nextIPPacket(&pak)) == 1))
    {
        logStackTrace("NaiveTCPMessageProvider::next", ret);
        logMessage("NaiveTCPMessageProvider::next: "
            "1 second before next buffer allocation attempt.");
        sleep(1);
    }

    iferr (ret != 0)
    {  
        logStackTrace("NaiveTCPMessageProvider::next", ret);
        return 1;
    }

    // already done by watcher
    // pak.initM3();

    if ((res = snew KernelMessage(&pak)) == 0)
    {
        logAllocError("NaiveTCPMessageProvider::next");
        delete pak.header;
        return 2;
    }

    res->setType(KernelMessage::Type::DATAIN);
    
#ifdef ENABLE_DEBUG_LOG
    logDebug("NaiveTCPMessageProvider::next:");
    //enum Type { ACK, DATAIN, DATAIN_ACK, ACKOUT, DATAOUT, RETX, UNKNOWN };
    KernelMessageMetaData::Type type = res->getType();
    logDebug("  Type: %s", KernelMessageMetaData::typeString[type]);
    PacketReader::packetDecode(pak.getIPHeader(), pak.getIPLen());
    PacketReader::m3Decode((DataMessageHeader*)pak.dataHeader);
#endif

    *dest = res;
    return 0;
}

}
