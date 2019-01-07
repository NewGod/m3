#ifndef __DEVICEHANDLER_HH__
#define __DEVICEHANDLER_HH__

#include <time.h>

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
    virtual int sendPacket(void *buf, int len) = 0;
    virtual int recvPacket(Packet *dest, bool check = true) = 0;
};

}

#endif
