#include "IPPacket.hh"
#include "Log.h"
#include "NetworkUtils.h"

namespace m3
{

void IPPacket::initRaw()
{
    iphdr *iph = (iphdr*)((unsigned long)this->header + ipoffset);
    unsigned iplen = ntohs(iph->tot_len);
    unsigned saddr = iph->saddr;
    unsigned daddr = iph->daddr;
    tcphdr *tcph = getTCPHeader(iph);
    DataMessageHeader *dh = 
        containerOf(tcph, sport, m3::DataMessageHeader);
    dh->saddr = saddr;
    dh->daddr = daddr;
    dh->tlen = iplen - (iph->ihl << 2) + DataMessageHeader::TCPH_OFF;
    dh->type = 0;
    dh->subtype = 0;
    dh->generateChk();

    logDebug("IPPacket::initRaw: tcplen %d = %d - %d", dh->getTCPLen(), iplen, 
        (int)((unsigned long)((char*)tcph + (tcph->doff << 2)) - 
        (unsigned long)iph));

    this->dataHeader = (char*)dh;
    this->type = Type::M3;
}

void IPPacket::initM3()
{
    tcphdr *tcph = getTCPHeader(getIPHeader());
    this->dataHeader = (char*)tcph + (tcph->doff << 2);
    this->type = Type::M3;
}

// need modification
ethhdr* IPPacket::toRaw(DeviceMetadata *src, DeviceMetadata *nexthop)
{
    static std::atomic<unsigned short> idcounter(0);
    DataMessageHeader *dh = (DataMessageHeader*)dataHeader;
    tcphdr* tcph = dh->getTCPHeader();
    iphdr *iph = (iphdr*)((unsigned long)tcph - M3_IP_HEADER_LEN);
    ethhdr *ethh = (ethhdr*)((char*)iph - sizeof(ethhdr));
    psdhdr psd;
    unsigned iplen = htons(dh->tlen - DataMessageHeader::TCPH_OFF + 
        M3_IP_HEADER_LEN);
    unsigned daddr = dh->daddr;

    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = iplen;
    iph->id = htons(idcounter++);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0;
    iph->saddr = src->ip;
    iph->daddr = daddr;

    psd.daddr = iph->daddr;
    psd.saddr = iph->saddr;
    psd.tcpl = htons(ntohs(iph->tot_len) - (iph->ihl << 2));
    psd.mbz = 0;
    psd.ptcl = IPPROTO_TCP;

    computeIPChecksum(iph);
    tcph->check = 0;
    tcph->check = tcp_cksum(&psd, tcph, ntohs(psd.tcpl));

    memcpy(ethh->h_dest, nexthop->mac, ETH_ALEN);
    memcpy(ethh->h_source, src->mac, ETH_ALEN);
    ethh->h_proto = htons(ETH_P_IP);

    ipoffset = (int)((unsigned long)iph - (unsigned long)header);

    this->type = Type::RAW;

    return ethh;
}

void IPPacket::computeIPChecksum(iphdr *iph)
{
    iph->check = cksum((ip*)iph, M3_IP_HEADER_LEN);
}

}