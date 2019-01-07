#ifndef __PACKETBUFFERALLOCATOR_HH__
#define __PACKETBUFFERALLOCATOR_HH__

#include "BlockAllocator.hh"

namespace m3
{

class PacketBufferAllocator : public ConcurrentBlockAllocator
{
protected:
    static PacketBufferAllocator *instance;

    PacketBufferAllocator(): ConcurrentBlockAllocator() {}
public:
    inline static PacketBufferAllocator *getInstance()
    {
        return instance;
    }
};

}

#endif
