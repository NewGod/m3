#ifndef __IPPACKETCAPTURE_HH__
#define __IPPACKETCAPTURE_HH__

#include <netinet/ip.h>
#include <sys/time.h>

namespace m3
{

struct IPPacketCapture
{
    iphdr *iph;
    timespec ts;
};

}

#endif
