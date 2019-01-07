#include <cstdlib>

#include "Connection.hh"
#include "DataMessage.hh"
#include "DataMessageHeader.hh"
#include "KernelMessage.hh"
#include "Log.h"
#include "PacketBufferAllocator.hh"
#include "PacketReader.hh"
#include "Represent.h"

namespace m3
{

DataMessage::~DataMessage()
{
    if (tofree)
    {
        PacketBufferAllocator::getInstance()->release(pak.header);
    }
}

static inline int getlen(unsigned short *arr)
{
    arr[0] = ntohs(arr[0]);
    arr[1] = ntohs(arr[1]);
    arr[2] = ntohs(arr[2]);
    return ((arr[0] ^ arr[1]) && (arr[0] ^ arr[2])) ? 
        ((arr[1] ^ arr[2]) ? 0 : arr[1]) : arr[0];
}

IPPacket* DataMessage::toIPPacket()
{
    return &pak;
}

int DataMessage::write(FileStream *os)
{
    int ret;
    int writeLen = getWriteLen() - pos;
    char* cdh = pak.dataHeader;

#ifdef ENABLE_DEBUG_LOG
    logDebug("DataMessage::write:");
    PacketReader::m3Decode((DataMessageHeader*)pak.dataHeader);
#endif

    iferr ((ret = os->write(cdh + pos, writeLen)) < 0)
    {
        logStackTrace("DataMessage::write", ret);
    }
    else
    {
        pos += ret;
    }
    return ret;
}

int DataMessage::getWriteLen()
{
    return pak.len - (pak.dataHeader - pak.header);
}

}