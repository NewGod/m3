#include <arpa/inet.h>
#include <netinet/in.h>
#include <pcap/pcap.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "Connection.hh"
#include "InterfaceManager.hh"
#include "LivePCAP.hh"
#include "Log.h"
#include "NetworkUtils.h"
#include "ServerPacketSource.hh"
#include "Represent.h"

namespace m3
{

ServerPacketSource *ServerPacketSource::instance = 0;

Connection *ServerPacketSource::getConnection(unsigned short port)
{
    portLock.readLock();
    auto ite = revPortMap.find(port);
    if (ite == revPortMap.end())
    {
        portLock.readRelease();
        return 0;
    }
    portLock.readRelease();
    return ite->second;
}

int ServerPacketSource::next(IPPacket *dest)
{
    IPPacket res;
    int ret;
    tcphdr *tcph;
    iphdr *iph;
    Connection *conn;
    sockaddr_in *origin;

    iferr ((ret = PacketReader::nextIPPacket(&res)) != 0)
    {
        logStackTrace("ServerPacketSource::next", ret);
        return 1;
    }

#ifdef ENABLE_DEBUG_LOG
    packetDecode(res.getIPHeader(), res.getIPLen());
#endif

    tcph = res.getTCPHeader(iph = res.getIPHeader());
    iferr ((conn = getConnection(htons(tcph->dest))) == 0)
    {
        logError("ServerPacketSource::next: No connection working on port %d.", 
            htons(tcph->dest));
        PacketReader::packetDecode(iph, res.getIPLen());
        return 2;
    }
    origin = conn->getSource();

    tcph->dest = origin->sin_port;
    psd.daddr = iph->daddr = *(unsigned*)&origin->sin_addr;
    psd.saddr = iph->saddr;
    psd.tcpl = htons(ntohs(iph->tot_len) - (iph->ihl << 2));
    tcph->check = 0;
    tcph->check = tcp_cksum(&psd, tcph, ntohs(psd.tcpl));
    iph->check = 0;
    iph->check = cksum(iph, iph->ihl << 2);

    res.initRaw();

#ifdef ENABLE_DEBUG_LOG
    logDebug("ServerPacketSource::next:");
    PacketReader::packetDecode(res.getIPHeader(), res.getIPLen());
    PacketReader::m3Decode((DataMessageHeader*)res.dataHeader);
#endif

    *dest = res;

    return 0;
}

void ServerPacketSource::releasePort(unsigned short port)
{
    if (port - portBase >= portLen ||
        port < portBase)
    {
        logWarning("ServerPacketSource::releasePort: "
            "Releasing port %d which is out of range [%d, %d)",
            port, portBase, portBase + portLen);
        return;
    }
    port -= portBase; 

    int pos = port >> 5;
    int level = port - (pos << 5);

    portLock.writeLock();
    portMask[pos] &= ~(1 << level);
    revPortMap[port] = 0;
    portLock.writeRelease();
}

int ServerPacketSource::allocPort(Connection *conn)
{
    unsigned short port = 0;
    portLock.readLock();

    for (int i = last + 1; i < portLen >> 5; ++i)
    {
        if (portMask[i] != 0xFFFFFFFF)
        {
            int mask = portMask[i], level;
            for (level = 0; mask & 1; mask >>= 1, level++);
            portLock.readRelease();

            portLock.writeLock();
            portMask[i] |= 1 << level;
            port = (i << 5) + level + portBase;
            conn->setProxyPort(port);
            revPortMap[port] = conn; 
            last = i;
            portLock.writeRelease();

            return 0;
        }
    }

    for (int i = 0; i < last; ++i)
    {
        if (portMask[i] != 0xFFFFFFFF)
        {
            int mask = portMask[i], level;
            for (level = 0; mask & 1; mask >>= 1, level++);
            portLock.readRelease();

            portLock.writeLock();
            portMask[i] |= 1 << level;
            port = (i << 5) + level + portBase;
            conn->setProxyPort(port);
            revPortMap[port] = conn; 
            last = i;
            portLock.writeRelease();

            return 0;
        }
    }

    portLock.readRelease();

    logError("ServerPacketSource::allocPort: No available port.");

    return 1;
}

int ServerPacketSource::forward(DataMessage *msg)
{
    Connection *conn = msg->getConnection();
    unsigned short port = conn->getProxyPort();
    IPPacket *pak = msg->toIPPacket();
    DataMessageHeader *dh = (DataMessageHeader*)pak->dataHeader;
    int ret;

    if (port == 0)
    {
        int ret;

        // allocate a port for this connection
        if ((ret = allocPort(conn)) != 0)
        {
            logStackTrace("ServerPacketSource::forward", ret);
            return 1;
        }
        logMessage("port %d", conn->getProxyPort());
    }

    dh->sport = htons(conn->getProxyPort());
    dh->dport = conn->getDest()->sin_port;

#ifdef ENABLE_DEBUG_LOG
    int tcplen = dh->getTCPLen();
    logDebug("ServerPacketSource::forward:");
    PacketReader::m3Decode((DataMessageHeader*)pak->dataHeader);
#endif

    ethhdr *ethh = pak->toRaw(&this->handle->me, &this->peer);

#ifdef ENABLE_DEBUG_LOG
    if (tcplen != 0)
    {
        PacketReader::packetDecode(pak->getIPHeader(), pak->getIPLen());
    }
#endif

    logDebug("pak->len %d, %lx %lx", pak->len,
        (unsigned long)ethh, (unsigned long)pak->header);

    iferr ((ret = this->handle->sendPacket(ethh, 
        pak->len - ((char*)ethh - pak->header))) != 0)
    {
        logStackTrace("ServerPacketSource::forward", ret);
        return 2;
    }

    return 0;
}

int ServerPacketSource::init(void *arg)
{   
    int ret;
    InitObject *iarg = (InitObject*)arg;
    char filter[128];
    char scriptargs[128];

    this->portBase = iarg->portBase;
    this->portLen = iarg->portLen;
    if ((this->portMask = snew unsigned[(this->portLen >> 5) + 1]) == 0)
    {
        logAllocError("ServerPacketSource::init");
        return 1;
    }

    sprintf(filter, "tcp and tcp dst portrange %d-%d and ! src %s",
        this->portBase, this->portBase + this->portLen - 1, 
        inet_ntoa(*(in_addr*)&(iarg->initPI.fwdip)));

    logMessage("SPS::init: filter %s", filter);

    iarg->initPI.filter = filter;

    sprintf(scriptargs, "-m -p %d-%d", 
        this->portBase, this->portBase + this->portLen - 1);
    iarg->initPI.scriptArgs = scriptargs;
    
    iferr ((ret = PacketInterceptor::init(&iarg->initPI)) != 0)
    {
        logStackTrace("ServerPacketSource::init", ret);
        return 2;
    }

    psd.mbz = 0;
    psd.ptcl = IPPROTO_TCP;

    return 0;
}

}