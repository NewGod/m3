#ifndef __RAWSOCKET_HH__
#define __RAWSOCKET_HH__

#include "DeviceHandle.hh"

namespace m3
{

class RawSocket : public DeviceHandle
{
public:
    RawSocket(): DeviceHandle(), fd(0), ifid(0) {}

    static const int MAX_IP_PACKET = 65536;

    int init(const char *device);
    virtual int sendPacket(void *buf, int len);
    virtual int recvPacket(Packet *dest, bool check = true);
    virtual iphdr* getiphdr(Packet *packet, bool check = true);
protected:
    int fd;
    int ifid;
    char buf[MAX_IP_PACKET];
};

}

#endif
