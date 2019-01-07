#ifndef __PACKETSOURCE_HH__
#define __PACKETSOURCE_HH__

#include "DataMessage.hh"
#include "IPPacket.hh"

namespace m3
{

class PacketSource
{
public:
    PacketSource() {}
    virtual ~PacketSource() {}

    virtual int init(void *arg) = 0;
    virtual int next(IPPacket *dest) = 0;
    virtual int forward(DataMessage *msg) = 0;
};

}

#endif
