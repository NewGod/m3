#ifndef __IPPACKET_HH__
#define __IPPACKET_HH__

#include <arpa/inet.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <atomic>

#include "DataMessageHeader.hh"
#include "DeviceMetadata.hh"
#include "Represent.h"

namespace m3
{

class IPPacket
{
public:
    IPPacket(): len(0) {}

    enum Type { RAW, M3, count };
    static const int M3_IP_HEADER_LEN = 20;
    static const int MAX_TCPHDR = 60;
    static const int ETH_OFFSET = 14;

    // eth & IP & TCP Header
    char *header;
    // M3 Data Header
    char *dataHeader;
    int len;
    int ipoffset;
    Type type;

    inline iphdr* getIPHeader()
    {
        return (iphdr*)(header + ipoffset);
    }
    inline tcphdr* getTCPHeader(iphdr *iph)
    {
        return (tcphdr*)((unsigned long)iph + (iph->ihl << 2));
    }
    inline tcphdr* getTCPHeader()
    {
        iphdr *iph = getIPHeader();
        return getTCPHeader(iph);
    }
    inline char* getTCPData(tcphdr *tcph)
    {
        return (char*)tcph + (tcph->doff << 2);
    }
    inline char* getTCPData()
    {
        tcphdr *tcph = getTCPHeader();
        return getTCPData(tcph);
    }
    inline int getIPLen()
    {
        return len - ipoffset;
    }
    inline int getTCPLen(tcphdr *tcph)
    {
        return len - ((char*)tcph - header) - (tcph->doff << 2);
    }
    inline int getTCPLen()
    {
        tcphdr *tcph = getTCPHeader(getIPHeader());
        return getTCPLen(tcph);
    }
    inline bool isRawPacket()
    {
        return type == RAW;
    }
    void initRaw();
    void initM3();
    ethhdr* toRaw(DeviceMetadata *src, DeviceMetadata *nexthop);
    static void computeIPChecksum(iphdr *iph);
};

}

#endif
