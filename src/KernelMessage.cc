#include <sys/socket.h>
#include <sys/types.h>

#include "DataMessageHeader.hh"
#include "KernelMessage.hh"
#include "PacketBufferAllocator.hh"
#include "Represent.h"

namespace m3
{

KernelMessage::~KernelMessage()
{
    if (tofree)
    {
        logDebug("ptr %lx", (unsigned long)pak.header);
        PacketBufferAllocator::getInstance()->release(pak.header);
    }
}

const char* KernelMessageMetaData::typeString[] = 
{
    "ACK",
    "DATAIN",
    "DATAIN_ACK",
    "ACKOUT",
    "DATAOUT",
    "RETX",
    "UNKNOWN"
};

}