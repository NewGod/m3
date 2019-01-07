#ifndef __FORWARDMODULE_HH__
#define __FORWARDMODULE_HH__

#include <sys/types.h>

#include <list>
#include <thread>

#include "BlockQueueBuffer.hh"
#include "KernelMessageProvider.hh"

namespace m3
{

class ConnectionManager;
class Interface;
class AbstractInternalMessageParser;
class KernelMessage;
class Message;
class ConnectionClassifier;
class PacketSource;

class ForwardModule
{
public:
    struct InitObject
    {
        int fd;
        KernelMessageProvider *kmp;
        Interface *master;
        ConnectionManager *connmgr;
        ConnectionClassifier *clasf;
        PacketSource *source;

        InitObject()
        {
            memset(this, 0, sizeof(InitObject));
        }
    };

    class KernelMessageWorker : public std::thread
    {
    protected:
        static void tmain(
            KernelMessageWorker *kmw, 
            ConnectionManager *cm, 
            Interface *interface);
        BlockPointerQueueBuffer<KernelMessage> kmbuf;
    public:
        inline KernelMessageWorker(
            ConnectionManager *cm, 
            Interface *interface): 
            std::thread(tmain, this, cm, interface), kmbuf() {}
        inline void advance(KernelMessage *msg)
        {
            kmbuf.advance(&msg);
        }
    };
    class ForwardWorker : public std::thread
    {
    protected:
        static void tmain(
            KernelMessageProvider *kmp, 
            ConnectionClassifier *clasf, 
            PacketSource *source, 
            AbstractInternalMessageParser *imp,
            KernelMessageWorker *kworker, 
            Interface *inf, 
            ConnectionManager *connmgr);
    public:
        inline ForwardWorker(
            KernelMessageProvider *kmp, 
            ConnectionClassifier *clasf, 
            PacketSource *source, 
            AbstractInternalMessageParser *imp, 
            KernelMessageWorker *kworker, 
            Interface *inf, 
            ConnectionManager *connmgr) : 
            std::thread(tmain, kmp, clasf, 
                source, imp, kworker, inf, connmgr) {}
    };
protected:
    Interface *master;
    AbstractInternalMessageParser *imp;
    KernelMessageWorker *kmw;
    ForwardWorker *fw;

    ConnectionManager *connmgr;
    ConnectionClassifier *clasf;
    PacketSource *source;
    KernelMessageProvider *kmp;
    int fd;
public:
    ForwardModule(): imp(0), kmw(0), fw(0) {}
    virtual ~ForwardModule();

    virtual int init(void *arg);
};

}

#endif

