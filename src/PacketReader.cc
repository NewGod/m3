#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <pcap/pcap.h>

#include <cstdlib>
#include <cstring>

#include "DataMessageHeader.hh"
#include "DeviceHandleInitializer.hh"
#include "IPPacket.hh"
#include "Log.h"
#include "NetworkUtils.h"
#include "PacketBufferAllocator.hh"
#include "PacketFilter.hh"
#include "PacketReader.hh"
#include "PCAP.hh"
#include "Reflect.hh"

namespace m3
{

PacketReader::~PacketReader()
{
    if (handle)
    {
        delete handle;
    }
}

int PacketReader::init(void *arg)
{
    int ret;
    InitObject *iobj = (InitObject*)arg;

    iferr ((ret = initHandler(iobj->handletype, iobj->nsname, 
        iobj->device, iobj->handlepriv)) != 0)
    {
        logStackTrace("PacketReader::init", ret);
        return 1;
    }
    setHeaderOffset(iobj->headerOffset);

    iferr ((ret = tsobuf.init(64)) != 0)
    {
        logStackTrace("PacketReader::init", ret);
        return 2;
    }
    iferr ((ret = tsolen.init(64)) != 0)
    {
        logStackTrace("PacketReader::init", ret);
        return 3;
    }

    iferr ((filter = snew PacketFilter()) == 0)
    {
        logAllocError("PacketReader::init");
        return 4;
    }

    iferr (iobj->filter != 0 && (ret = filter->init(iobj->filter)) != 0)
    {
        logStackTrace("PacketReader::init", ret);
        return 5;
    }

    return 0;
}

void PacketReader::packetDecode(iphdr *iph, int len)
{
    ethhdr *ethh = (ethhdr*)((char*)iph - sizeof(ethhdr));
    tcphdr *tcph = (tcphdr*)((char*)iph + (iph->ihl << 2));
    char src[32];
    char dst[32];

    logMessage("  ether: src %s, dst %s proto 0x%04x", 
        ether_ntoa_r((const ether_addr*)ethh->h_source, src),
        ether_ntoa_r((const ether_addr*)ethh->h_dest, dst), ethh->h_proto);

    logMessage("  ip: ver %d, ihl %d, tos %d, len %d, id %d, frag %d, ttl %d "
        "proto %d, check 0x%04x, src %s, dst %s", iph->version, iph->ihl, 
        iph->tos, ntohs(iph->tot_len), ntohs(iph->id), 
        ntohs(iph->frag_off), iph->ttl, iph->protocol, iph->check,
        inet_ntoa_r(*(in_addr*)&iph->saddr, src), 
        inet_ntoa_r(*(in_addr*)&iph->daddr, dst));

    logMessage("  tcp: src %d, dst %d, seq %u, ack %u, flags %s, window %d, "
        "check 0x%04x, urg %d, doff %d, data %02x", 
        ntohs(tcph->source), ntohs(tcph->dest), 
        ntohl(tcph->seq), ntohl(tcph->ack_seq), 
        dump_tcp_flags(tcph, src), ntohs(tcph->window), tcph->check,
        ntohs(tcph->urg_ptr), tcph->doff, *((u_char*)tcph + (tcph->doff << 2)));
}

void PacketReader::m3Decode(DataMessageHeader *dh)
{
    char src[16];
    char dst[16];
    char flags[16];

    logMessage("  m3: type %d, subtype %d, tlen %u(%u, %u), saddr %s, "
        "daddr %s, sport %d, dport %d, seq %u, ack %u, flags %s, window %d, "
        "check %04x, urg %d", dh->type, dh->subtype, dh->tlen,
        (unsigned short)(dh->lenchk1 + DataMessageHeader::magic1),
        (unsigned short)(dh->lenchk2 - DataMessageHeader::magic2),
        inet_ntoa_r(*(in_addr*)&dh->saddr, src),
        inet_ntoa_r(*(in_addr*)&dh->daddr, dst), 
        ntohs(dh->sport), ntohs(dh->dport), ntohl(dh->seq), ntohl(dh->ackseq), 
        dump_tcp_flags(&dh->tcph, flags), ntohs(dh->window), 
        dh->check, dh->urgptr);
}

int PacketReader::initHandler(const char *type, const char *nsname,
    const char *device, void *priv)
{
    std::promise<int> ret;
    std::future<int> retval = ret.get_future();
    DeviceHandleInitializer *initializer = (DeviceHandleInitializer*)
        Reflect::create(type);
    
    iferr (initializer == 0)
    {
        logStackTrace("PacketReader::initHandler", initializer);
        return 1;
    }

    initializer->run(&ret, nsname, device, priv);
    int res = retval.get();
    initializer->join();
    handle = initializer->getHandle();

    iferr (res != 0)
    {
        logStackTrace("PacketReader::initHandler", res);
        return 2;
    }

    return 0;
}

int PacketReader::nextPacketWithTSO(u_char **data, int *len)
{
    Packet packet;
    int ret;

    while (tsobuf.isEmpty())
    {
        PacketBufferAllocator *allocator =
            PacketBufferAllocator::getInstance();
        iphdr *iph;
        tcphdr *tcph;
        u_char *tcpdata;
        int hlen;
        int tcplen;
        int remlen;
        int thlen;
        int i;

        iferr ((ret = handle->recvPacket(&packet)) != 0)
        {
            logStackTrace("PacketReader::nextPacketWithTSO", ret);
            return 1;
        }

        lastCapture = packet.timestamp;

        if ((iph = handle->getiphdr(&packet)) == 0)
        {
            continue;
        }
#ifdef ENABLE_DEBUG_LOG
        packetDecode(iph, 0);
#endif
        tcph = (tcphdr*)((char*)iph + (iph->ihl << 2));
        tcpdata = (u_char*)tcph + (tcph->doff << 2);
        hlen = tcpdata - (u_char*)iph;
        tcplen = ntohs(iph->tot_len) - hlen;
        if (filter != 0 && filter->drop(iph))
        {
            if (tcph->syn == 1)
            {
                logMessage("SYNCAP");
            }
            continue;
        }

        if (tcplen <= mss)
        {
            u_char *buf = (u_char*)allocator->allocate();
            
            iferr (buf == 0)
            {
                logAllocError("PacketReader::nextPacketWithTSO");
                return 2;
            }
            
            *len = ntohs(iph->tot_len);
            *data = buf;
            memcpy(buf + headerOffset, iph, *len);
            return 0;
        }

        remlen = tcplen;
        thlen = tcph->doff << 2;
        i = 0;
        do
        {
            u_char *buf = (u_char*)allocator->allocate();
            iphdr *biph = (iphdr*)((unsigned long)buf + headerOffset);
            tcphdr *btcph = (tcphdr*)((unsigned long)buf + 
                headerOffset + IPPacket::M3_IP_HEADER_LEN);
            unsigned short tlen;
            int ilen;

            remlen = tcplen - i;

            iferr (buf == 0)
            {
                int frag = 0;
                if (mss != 0)
                {
                    frag = (remlen + mss - 1) / mss;
                }
                logAllocError("PacketReader::nextPacketWithTSO");
                logWarning("Ignoring %d packet fragments", frag);
                if (i == 0)
                {
                    return 3;
                }
                break;
            }

            ilen = remlen < mss ? remlen : mss;
            
            tlen = ilen + thlen + IPPacket::M3_IP_HEADER_LEN;

            memcpy(btcph, tcph, thlen);
            memcpy((char*)btcph + thlen, tcpdata + i, ilen);

            biph->ihl = IPPacket::M3_IP_HEADER_LEN >> 2;
            biph->tot_len = htons(tlen);
            biph->saddr = iph->saddr;
            biph->daddr = iph->daddr;

            btcph->seq = htonl(ntohl(tcph->seq) + i);
            btcph->fin = tcph->fin && (i + ilen >= tcplen);
            
            tsobuf.push(buf);
            tsolen.push(tlen);

            i += ilen;
        }
        while (i < tcplen);
    }

    *data = tsobuf.pop();
    *len = tsolen.pop();

    return 0;
}

int PacketReader::nextIPPacket(IPPacket *dest)
{
    u_char *buf;
    int ret;
    int offset = headerOffset;
    int len;

    iferr ((ret = nextPacketWithTSO(&buf, &len)) != 0)
    {
        logStackTrace("PacketReader::nextIPPacket", ret);
        return 2;
    }

    logDebug("data %lx, plen %d, headeroff %d", 
        (unsigned long)buf, len, offset);

    dest->header = (char*)buf;
    dest->len = len + offset;
    dest->ipoffset = offset;
    buf[len + offset] = 0;
    
    return 0;
}

}