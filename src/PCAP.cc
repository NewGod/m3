#include "Log.h"
#include "PCAP.hh"
#include "Represent.h"

namespace m3
{

PCAP::~PCAP()
{
    if (source)
    {
        pcap_close(source);
    }
}

iphdr* PCAP::getiphdr(Packet *packet, bool check)
{
    void *buf = packet->buf;
	iphdr* iph = (iphdr*)packet->buf;
    int len = packet->len;
    int totlen;

    if (offset != 0)
    {
        int ether_type = ntohs(*((uint16_t *)((char*)buf + (offset - 2))));

        if (ether_type != ETHERTYPE_IP)
        {
            // keep silent for ARP & IPv6 packets
            if (ether_type != ETHERTYPE_IPV6 && ether_type != ETHERTYPE_ARP)
            {
                logWarning("PCAP::getiphdr: Unrecognized ether type %x",
                    (unsigned)ether_type);
            }
            return 0;
        }
        iph = (iphdr*)((char*)buf + offset);
    }

    if (!check)
    {
        return iph;
    }

    iferr (((char*)iph - (char*)buf) + offsetOf(tot_len, iphdr) > len ||
        ((char*)iph - (char*)buf) + (totlen = ntohs(iph->tot_len)) > len)
    {
        logError("PCAP::getiphdr: IP header length error(%d, %d, %d).", len,
            offset, totlen);
        return 0;
    }

    tcphdr *tcph = (tcphdr*)((char*)iph + (iph->ihl << 2));
    iferr (((char*)tcph) - (char*)buf + offsetOf(ack_seq, tcphdr) + 4 > len ||
        ((char*)tcph) - (char*)buf + (tcph->doff << 2) > len)
    {
        logError("PCAP::getiphdr: Truncated TCP header.");
        return 0;
    }

    return iph;
}

int PCAP::assignFilter(const char *filter)
{
    bpf_program prog;
    iferr (pcap_compile(
        source, &prog, filter, 1, PCAP_NETMASK_UNKNOWN) != 0)
    {
        logError("PCAP::assignFilter: pcap_compile failed(%s).", 
            pcap_geterr(source));
        logError("  Filter text: %s", filter);
        return 1;
    }
    iferr (pcap_setfilter(source, &prog) != 0)
    {
        logError("PCAP::assignFilter: pcap_setfilter failed(%s).", 
            pcap_geterr(source));
        return 2;
    }

    return 0;
}

int PCAP::recvPacket(Packet *dest, bool check)
{
    pcap_pkthdr *pkt_header = 0;
    const u_char *pkt_data = 0;
    int ret;

    //logMessage("PCAP::nextPacket: capture");
    while ((ret = pcap_next_ex(source, &pkt_header, &pkt_data)) == 0);
    iferr (ret != 1)
    {
        logError("PCAP::nextPacket: pcap_next_ex failed(%d, %s)", 
            ret, pcap_geterr(source));
        return 1;
    }
    //logMessage("PCAP::nextPacket: captured");

    logDebug("PCAP::nextPacket: "
        "Packet %lx captured: Time(%ld.%ld), caplen(%d), len(%d)",
        (unsigned long)pkt_data,
        pkt_header->ts.tv_sec, pkt_header->ts.tv_usec, 
        pkt_header->caplen, pkt_header->len);

    if (!check)
    {
        goto ret_succ;
    }

    iferr (pkt_header->len != pkt_header->caplen)
    {
        logError("PCAP::nextPacket: Packet truncated(length = %d, "
            "expected %d", pkt_header->caplen, pkt_header->len);
        return 2;
    }

ret_succ:
    dest->len = pkt_header->len;
    dest->buflen = pkt_header->caplen;
    dest->timestamp.tv_sec = pkt_header->ts.tv_sec;
    dest->timestamp.tv_nsec = pkt_header->ts.tv_usec * 1000l;
    dest->buf = (void*)pkt_data;
    return 0;
}

int PCAP::sendPacket(void *buf, int len)
{
    int ret;

    logDebug("PCAP::sendPacket: Sending %d bytes", len);

    iferr ((ret = pcap_inject(source, buf, len)) < 0)
    {
        logError("PCAP::sendPacket: pcap_inject failed(%s)", 
            pcap_geterr(source));
        return 1;
    }

    return 0;
}

}