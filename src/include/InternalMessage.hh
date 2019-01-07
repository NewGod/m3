#ifndef __INTERNALMESSAGE_HH__
#define __INTERNALMESSAGE_HH__

#include <thread>

#include "Connection.hh"
#include "Context.hh"
#include "DataMessage.hh"
#include "DataMessageHeader.hh"
#include "InternalMessageParser.hh"
#include "PacketBufferAllocator.hh"
#include "SpinQueueBuffer.hh"

namespace m3
{

class ConnectionManager;
class ContextManager;
class PolicyManager;

// How to send an InternalMessage
//   1. call getMessageBuf() to allocate buffer
//   2. fill buffer with your message payload(user MUST ensure length <= mss)
//   3. construct an InternalMessage with the buffer you allocated
//   4. call sendInternalMessage() with the InternalMessage you constructed
//   5. free the message manually iff sendInternalMessage() failed.
//   *: releaseMessageBuf() provides a way to release buffer before an
//      InternalMessage was constructed. if you has constructed an 
//      InternalMessage, simply `delete` it, buffer will be automatically 
//      released.
// How to receive an InternalMessage
//   1. call ContextManager::getInstance()->getContext() with type = 
//      Context::Type::INTERNAL to get InternalMessageContext instance.
//   2. call tryPurchase() on the instance to access the messages one by one in 
//      the pending message queue.
//   *: messages are sorted by their receiving sequence. user MUST NOT assume 
//      messages to be arrived in order. 
//   **: if the queue was completely filled, upon new arrivals oldest message 
//      will be discarded. default queue size is 2^12. 
//   ***: The message consumer is in charge of deleting messages purchased by 
//      themselves.
class InternalMessage : public DataMessage
{
protected:
    static Connection *conn;
public:
    InternalMessage(void *buf, int len, 
        DataMessageMetaHeader::InternalSubtype subtype = 
        DataMessageMetaHeader::InternalSubtype::I_GENERIC);
    static inline void* getMessageBuf()
    {
        void *buf = PacketBufferAllocator::getInstance()->allocate();

        return buf == 0 ? buf : (char*)buf + sizeof(DataMessageMetaHeader);
    }
    static inline void releaseMessageBuf(void* buf)
    {
        PacketBufferAllocator::getInstance()->release((char*)buf - 
            sizeof(DataMessageMetaHeader));
    }
    static int initInternalMessage(ConnectionManager *connmgr,
        PolicyManager *plcmgr);
    static inline int sendInternalMessage(InternalMessage *msg)
    {
        return conn->issue(msg);
    }
};

class InternalMessageContext : public Context, 
    public SpinPointerQueueBuffer<InternalMessage>
{
public:
#ifdef INTERNALMESSAGE_DEBUG
    InternalMessageContext(int level = 2): Context(),
        SpinQueueBuffer(level) {}
#else
    InternalMessageContext(int level = DEFLEVEL): Context(),
        SpinQueueBuffer(level) {}
#endif
    virtual Type getType() override
    {
        return Type::INTERNAL;
    }
};

}

#endif

