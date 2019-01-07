#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/if.h>
#include <linux/if_packet.h>

#include "Log.h"
#include "PacketBufferAllocator.hh"
#include "RawSocket.hh"
#include "Represent.h"

namespace m3
{
/*
char interface_name[12];
strcpy(interface_name, "eth1");
sd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
if ( sd == -1 ) {
    perror("error in opening socket.");
    return;
}

if ( setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, interface_name, strlen(interface_name)) == -1 )
{
    perror("error in binding sd.");
    return;
}
int one = 1;
if ( setsockopt (sd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0)
{
    perror("error in setting HDRINCL.");
    return;
}
struct ifreq ifr;
memset(&ifr, 0, sizeof(ifr));
strcpy(ifr.ifr_name, interface_name);
if (ioctl(sd, SIOCGIFINDEX, &ifr) < 0) {
    perror("ioctl(SIOCGIFINDEX) failed");
    return;
}
int interface_index = ifr.ifr_ifindex;
ifr.ifr_flags |= IFF_PROMISC;
if( ioctl(sd, SIOCSIFFLAGS, &ifr) != 0 )
{
    perror("ioctl for IFF_PROMISC failed.");
    return;
}
struct packet_mreq mr;
memset(&mr, 0, sizeof(mr));
mr.mr_ifindex = interface_index;
mr.mr_type = PACKET_MR_PROMISC;

if (setsockopt(sd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) {
    perror("setsockopt(PACKET_MR_PROMISC) failed");
    return 1;
}
*/
int RawSocket::init(const char *device)
{
    int ret;
    int len;

    iferr ((fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0)
    {
        char errbuf[64];
        logError("RawSocket::init: socket() failed(%s)", 
            strerrorV(errno, errbuf));
        return 1;
    }

    iferr ((len = strnlen(device, IFNAMSIZ)) == IFNAMSIZ)
    {
        logError("RawSocket::init: Interface name too long.");
        return 2;
    }

    iferr (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, device, len) < 0)
    {
        char errbuf[64];
        logError("RawSocket::init: Cannot bind to device(%s)", 
            strerrorV(errno, errbuf));
        return 3;
    }

    iferr ((ret = this->me.setMetadata(device)) != 0)
    {
        logStackTrace("RawSocket::init", ret);
        return 5;
    }


    ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, device);
    iferr (ioctl(fd, SIOCGIFINDEX, &ifr) < 0)
    {
        char errbuf[64];
        logError("RawSocket::init: ioctl(SIOCGIFINDEX) failed(%s)", 
            strerrorV(errno, errbuf));
        return 6;
    }
    ifid = ifr.ifr_ifindex;
    ifr.ifr_flags |= IFF_PROMISC;
    ifr.ifr_flags |= IFF_UP;
    ifr.ifr_flags |= IFF_LOWER_UP;
    iferr (ioctl(fd, SIOCSIFFLAGS, &ifr) != 0)
    {
        char errbuf[64];
        logError("RawSocket::init: ioctl(SIOCSIFFLAGS) failed(%s)", 
            strerrorV(errno, errbuf));
        return 7;
    }
    packet_mreq mr;
    memset(&mr, 0, sizeof(mr));
    mr.mr_ifindex = ifid;
    mr.mr_type = PACKET_MR_PROMISC;
    iferr (setsockopt(fd, SOL_PACKET, 
        PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) < 0) 
    {
        char errbuf[64];
        logError("RawSocket::init: setsockopt(PACKET_MR_PROMISC) failed(%s)", 
            strerrorV(errno, errbuf));
        return 8;
    }

    return 0;
}

int RawSocket::sendPacket(void *buf, int len)
{
    ethhdr *ethh = (ethhdr*)buf;
    sockaddr_ll addr;
    memset(&addr, 0, sizeof(sockaddr_ll));
    
    addr.sll_ifindex = ifid; // index of interface
    addr.sll_halen = ETH_ALEN; // length of destination mac address
    addr.sll_addr[0] = ethh->h_dest[0];
    addr.sll_addr[1] = ethh->h_dest[1];
    addr.sll_addr[2] = ethh->h_dest[2];
    addr.sll_addr[3] = ethh->h_dest[3];
    addr.sll_addr[4] = ethh->h_dest[4];
    addr.sll_addr[5] = ethh->h_dest[5];

    if (sendto(fd, buf, len, 0, (sockaddr*)&addr, (socklen_t)sizeof(addr)) < 0)
    {
        char errbuf[64];
        logError("RawSocket::sendPacket: sendto() failed(%s)",
            strerrorV(errno, errbuf));
        return 1;
    }

    return 0;
}

int RawSocket::recvPacket(Packet *dest, bool check)
{
    int buflen = recvfrom(fd, buf, MAX_IP_PACKET, 0, 0, 0);

    if (buflen < 0)
    {
        char errbuf[64];
        logError("RawSocket::recvPacket: recvFrom() failed(%s).", 
            strerrorV(errno, errbuf));
        return 1;
    }

    dest->len = dest->buflen = buflen;
    dest->buf = buf;
    clock_gettime(CLOCK_REALTIME_COARSE, &dest->timestamp);
    return 0;
}

iphdr *RawSocket::getiphdr(Packet *packet, bool check)
{
    return (iphdr*)((char*)packet->buf + sizeof(ethhdr));
}

}