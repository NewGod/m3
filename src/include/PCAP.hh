#ifndef __PCAP_HH__
#define __PCAP_HH__

#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <pcap/pcap.h>

#include "DeviceHandle.hh"

namespace m3
{

class PCAP : public DeviceHandle
{
public:
    static const int MAX_IP_PACKET_LEN = 65536;
    pcap_t *source;
    int offset;
    char errbuf[PCAP_ERRBUF_SIZE];
    PCAP(): DeviceHandle(), source(0) {}
    virtual ~PCAP();
    virtual int init(const void *arg) = 0;
    virtual iphdr* getiphdr(Packet *packet, bool check = true) override;
    int assignFilter(const char *filter);
    virtual int recvPacket(Packet *dest, bool check = true) override;
    virtual int sendPacket(void *buf, int len) override;
};

#define LINKTYPE_ETHERNET 1
#define LINKTYPE_RAW 101
// ? I can't find this type on the man page
#define LINKTYPE_TUN 12
#define LINKTYPE_LINUX_SLL 113
#define ETHERTYPE_IP 0x0800

}

#endif
