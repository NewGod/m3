#ifndef __KERNELMESSAGE_HH__
#define __KERNELMESSAGE_HH__

#include "Message.hh"

namespace m3
{

class KernelMessageMetaData
{
public:
    enum Type { ACK, DATAIN, DATAIN_ACK, ACKOUT, DATAOUT, RETX, UNKNOWN, 
        count };
    static const char *typeString[];

    Type type;
};

// In this version, we use libpcap to capture packets and packets are the only
// data source we need from kernel. So although we define KernelMessage and
// DataMessage as different classes, their data structure is almost the same.
class KernelMessage : public Message
{
public:
    KernelMessage(Connection *ses = 0): 
        Message(ses), meta(), pak(), tofree(false) {}
    KernelMessage(IPPacket *pak, bool tofree = true, Connection *ses = 0): 
        Message(ses), meta(), pak(*pak), tofree(tofree) {}
    virtual ~KernelMessage();
    static void* newInstance() //@ReflectKernelMessage
    {
        return snew KernelMessage();
    }
    //const int MAX_KMSG_LEN = 2048;
    typedef KernelMessageMetaData::Type Type;
    
    //static int read(KernelMessage *dest, int ctrlfd);
    inline Type getType()
    {
        return meta.type;
    }
    inline void setType(Type type)
    {
        this->meta.type = type;
    }
    IPPacket* toIPPacket() override
    {
        return &pak;
    }
    void setToFree(bool tofree)
    {
        this->tofree = tofree;
    }
protected:
    // const value(once initilized, no more modification)
    KernelMessageMetaData meta;

    IPPacket pak;
    bool tofree;
};

}

#endif
