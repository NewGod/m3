#ifndef __DEVICEHANDLE_HH__
#define __DEVICEHANDLE_HH__

#include <time.h>

#include <netinet/ip.h>

#include "DeviceMetadata.hh"

namespace m3
{

// highly inspired by pcap_pkthdr
struct Packet
{
    int len;
    int buflen;
    timespec timestamp;
    void *buf;
    void *priv;

    Packet(): len(0), buflen(0), timestamp({0, 0}), buf(0), priv(0) {}
};

class DeviceHandle
{
public:
    DeviceHandle(): me() {}
    virtual ~DeviceHandle() {}

    DeviceMetadata me;
    virtual int sendPacket(void *buf, int len) = 0;
    virtual int recvPacket(Packet *dest, bool check = true) = 0;
    virtual iphdr* getiphdr(Packet *packet, bool check = true) = 0;
};

}

#endif
