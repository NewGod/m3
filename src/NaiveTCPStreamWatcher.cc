#include "DeviceHandle.hh"
#include "IPPacket.hh"
#include "IPPacketCapture.hh"
#include "NaiveTCPStreamWatcher.hh"
#include "NetworkUtils.h"
#include "PacketBufferAllocator.hh"

namespace m3
{

NaiveTCPStreamWatcher::~NaiveTCPStreamWatcher() {}

void NaiveTCPStreamWatcher::MetricWorker::tmain(
    NaiveTCPStreamWatcher *ntcpw)
{
    Packet packet;
    int ret;
    DeviceHandle *handle = ntcpw->handle;
    IPPacketCapture capture;

    logMessage("NaiveTCPStreamWatcher::MetricWorker::tmain: Thread identify");

    while (1)
    {
        iferr ((ret = handle->recvPacket(&packet, false)) != 0)
        {
            logStackTrace("NaiveTCPStreamWatcher::MetricWorker::tmain", ret);
            logStackTraceEnd();
            continue;
        }

        // ignore non-IPv4 packets
        if ((capture.iph = handle->getiphdr(&packet, false)) == 0)
        {
            continue;
        }
        
        capture.ts = packet.timestamp;

        ntcpw->renewMetric(&capture);
    }
}

int NaiveTCPStreamWatcher::init(void *arg)
{
    int ret;
    char filter[1024];
    char src[32];
    char dst[32];
    InitObject *iarg = (InitObject*)arg;
    sprintf(filter, "(tcp port %d or tcp port %d) and (host %s or host %s)", 
        ntohs(iarg->myaddr.sin_port), ntohs(iarg->peeraddr.sin_port),
        inet_ntoa_r(iarg->myaddr.sin_addr, src), 
        inet_ntoa_r(iarg->peeraddr.sin_addr, dst));

    logMessage("NaiveTCPStreamWatcher::init: filter %s", filter);
    iarg->prInit.filter = filter;
    iferr ((ret = PacketReader::init(&iarg->prInit)) != 0)
    {
        logStackTrace("NaiveTCPStreamWatcher::init", ret);
        return 1;
    }

    iferr ((ret = ArrayMetricProvider::init(MAX_METRIC)) != 0)
    {
        logStackTrace("NaiveTCPStreamWatcher::init", ret);
        return 2;
    }
    
    for (int i = 0; i < iarg->metricCount; ++i)
    {
        if ((ret = addMetric(iarg->metrics[i])) != 0)
        {
            logStackTrace("NaiveTCPStreamWatcher::init", ret);
            logMessage("NaiveTCPStreamWatcher::init: at %d", i);
            return 3;
        }
    }

    iferr ((ret = is.bind(this->fd = iarg->fd)) != 0)
    {
        logStackTrace("NaiveTCPStreamWatcher::init", ret);
        return 4;
    }

    iferr ((mw = snew MetricWorker(this)) == 0)
    {
        logAllocError("NaiveTCPStreamWatcher::init");
        return 6;
    }

    return 0;
}

#define readWithExceptionHandling(ret, buf, len, res)\
    iferr ((ret = is.read(buf, len)) < len)\
    {\
        if (ret < 0)\
        {\
            logStackTrace("NaiveTCPStreamWatcher::nextIPPacket", ret);\
        }\
        else\
        {\
            logError("NaiveTCPStreamWatcher::nextIPPacket: "\
                "Stream unexpectedly terminated, force exit.");\
            kill(getpid(), SIGTERM);\
            sleep(1000);\
        }\
        return res;\
    }

int NaiveTCPStreamWatcher::nextIPPacket(IPPacket *dest)
{
    static const int offset = headerOffset + IPPacket::M3_IP_HEADER_LEN -
        DataMessageHeader::TCPH_OFF;
    
    u_char *buf;
    int ret;
    PacketBufferAllocator *allocator = PacketBufferAllocator::getInstance();
    int bs;
    DataMessageHeader *dh;
    int dmlen;
    int rlen;

    iferr ((buf = (u_char*)allocator->allocate()) == 0)
    {
        logAllocError("NaiveTCPStreamWatcher::nextIPPacket");
        return 1;
    }

    readWithExceptionHandling(ret, buf + offset, DataMessageHeader::LEN_OFF, 2);
    dh = (DataMessageHeader*)(buf + offset);
    bs = allocator->getBlockSize();
    dmlen = dh->tlen;
    
    // provide zero terminate byte for functions like sscanf.
    ((char*)dh)[dmlen] = 0;
    
    // use weak check here because we have exactly no way to do recovery
    // and error will happen only when TCP data is wrong(with very low
    // possibility)
    iferr (!dh->weakCheck() || dmlen > bs - offset)
    {
        logError("NaiveTCPStreamWatcher::nextIPPacket: Synchronization lost!");
        m3Decode(dh);
        return 3;
    }

    rlen = dmlen - DataMessageHeader::LEN_OFF;
    readWithExceptionHandling(ret, buf + offset + DataMessageHeader::LEN_OFF, 
        rlen, 4);
    
    dest->header = (char*)buf;
    dest->len = dmlen + offset;
    dest->ipoffset = offset;
    dest->dataHeader = (char*)dh;
    dest->type = IPPacket::Type::M3;

    return 0;
}

}
