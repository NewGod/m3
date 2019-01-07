#ifndef __DATAMESSAGE_HH__
#define __DATAMESSAGE_HH__

#include <sys/time.h>

#include <cstdint>

#include "DataMessageHeader.hh"
#include "FileStream.hh"
#include "Message.hh"
#include "PacketBufferAllocator.hh"

namespace m3
{

class DataMessage : public Message
{
protected:
    DataMessage(Connection *ses = 0): Message(ses), pak(), pos(0),
        tofree(false) {}
    IPPacket pak;
    int pos;
    bool tofree;
    //timeval sendTime;
public:
    DataMessage(IPPacket *pak, bool tofree = true, Connection *ses = 0): 
        Message(ses), pak(*pak), pos(0), tofree(tofree) {}
    static void* newInstance() //@ReflectDataMessage
    {
        return snew DataMessage();
    }
    virtual ~DataMessage();
    int write(FileStream *os);
    IPPacket* toIPPacket() override;
    int getWriteLen();
    void setToFree(bool tofree)
    {
        this->tofree = tofree;
    }
    inline DataMessageHeader *getHeader()
    {
        return (DataMessageHeader*)pak.dataHeader;
    }
    inline bool written()
    {
        return pos == getWriteLen();
    }
};

}

#endif
 