#include "PacketBufferAllocator.hh"

namespace m3
{

PacketBufferAllocator *PacketBufferAllocator::instance = 
    snew PacketBufferAllocator();

}
