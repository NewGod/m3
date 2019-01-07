#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "Connection.hh"
#include "ConnectionClassifier.hh"
#include "ConnectionManager.hh"
#include "DataMessage.hh"
#include "ForwardModule.hh"
#include "Interface.hh"
#include "InternalMessageParser.hh"
#include "KernelMessage.hh"
#include "LivePCAP.hh"
#include "Log.h"
#include "NetworkUtils.h"
#include "PacketReader.hh"
#include "Policy.hh"
#include "Represent.h"

namespace m3
{

#define clearPointer(ptr)\
    if (ptr != 0)\
    {\
        delete ptr;\
        ptr = 0;\
    }

ForwardModule::~ForwardModule()
{
    clearPointer(imp);
}

int ForwardModule::init(void *arg)
{
    InitObject *iarg = (InitObject*)arg;

    this->master = iarg->master;
    this->connmgr = iarg->connmgr;
    this->clasf = iarg->clasf;
    this->source = iarg->source;
    this->kmp = iarg->kmp;

    iferr ((imp = snew InternalMessageParser()) == 0)
    {
        logAllocError("ForwardModule::init");
        return 1;
    }

    iferr ((kmw = snew KernelMessageWorker(iarg->connmgr, iarg->master)) == 0 ||
        (fw = snew ForwardWorker(kmp, iarg->clasf, iarg->source, 
        imp, kmw, iarg->master, iarg->connmgr)) == 0)
    {
        logAllocError("ForwardModule::init");
        return 2;
    }

    return 0;
}

void ForwardModule::ForwardWorker::tmain(KernelMessageProvider *kmp,
    ConnectionClassifier *clasf, PacketSource *source, 
    AbstractInternalMessageParser *imp, KernelMessageWorker *kworker, 
    Interface *inf, ConnectionManager *connmgr)
{
    KernelMessage *kmsg = 0;
    DataMessage *msg = 0;

    logMessage("ForwardModule::ForwardWorker::tmain: Thread identify");

    while (1)
    {
        IPPacket *pak;
        int ret;
        Connection *conn;
        KernelMessage::Type type;

        logDebug("ForwardModule::ForwardWorker::tmain: Waiting for message.");

        while (kmp->next(&kmsg));
        type = kmsg->getType();

        iferr (type == KernelMessage::Type::ACK || 
            type == KernelMessage::Type::ACKOUT ||
            type >= KernelMessage::Type::count)
        {
            // We have refreshed the metrics in KernelMessageProvider::next
            // so treat this as an error now.
            logError("ForwardModule::ForwardWorker::tmain: "
                "Unexpected message type %d", type);
            delete kmsg;
            continue;
        }

        pak = kmsg->toIPPacket();

        iferr ((ret = connmgr->getConnection(kmsg)) != 0)
        {
            logStackTrace("ForwardModule::ForwardWorker::tmain", ret);
            logStackTraceEnd();
            PacketReader::packetDecode(pak->getIPHeader(), pak->getIPLen());
            PacketReader::m3Decode((DataMessageHeader*)pak->dataHeader);
            delete kmsg;
            continue;
        }
        conn = kmsg->getConnection();

        logDebug("ForwardModule::ForwardWorker::tmain: Got message.");

        switch (type)
        {
        case KernelMessage::Type::DATAIN:
        case KernelMessage::Type::DATAIN_ACK:
            iferr ((msg = snew DataMessage(pak, false)) == 0)
            {
                logAllocError("ForwardModule::ForwardWorker::tmain");
                PacketReader::packetDecode(pak->getIPHeader(), pak->getIPLen());
                PacketReader::m3Decode((DataMessageHeader*)pak->dataHeader);
                delete kmsg;
                break;
            }
            iferr ((ret = msg->setConnection(conn)) != 0)
            {
                logStackTrace("ConnectionManager::getConnection", ret);
                logStackTraceEnd();
                PacketReader::packetDecode(pak->getIPHeader(), pak->getIPLen());
                PacketReader::m3Decode((DataMessageHeader*)pak->dataHeader);
                delete kmsg;
                delete msg;
                break;
            }
            if (imp->parse(msg) != 0)
            {
                clasf->detect(msg);
                source->forward(msg);
                logDebug("ForwardModule::ForwardWorker::tmain: "
                    "Forward complete.");
                delete msg;
            }
            else
            {
                // let message handler to free 
                kmsg->setToFree(false);
                msg->setToFree(true);
            }
            if (kmsg->getType() == KernelMessage::Type::DATAIN)
            {
                conn->renewOnInput(kmsg);
                delete kmsg;
                break;
            }
        default:
            // let kmessage worker process it
            logDebug("ForwardModule::ForwardWorker::tmain: Inserting %lx",
                (unsigned long)kmsg);
            kworker->advance(kmsg);
        }
        logDebug("ForwardModule::ForwardWorker::tmain: "
            "Parse complete.");
    }
}

void ForwardModule::KernelMessageWorker::tmain(KernelMessageWorker *kmw,
    ConnectionManager *connmgr, Interface *interface)
{
    KernelMessage *kmsg;
    Connection *conn;
    Policy *policy;

    logMessage("ForwardModule::KernelMessageWorker::tmain: Thread identify");

    while (1)
    {
        kmw->kmbuf.purchase(&kmsg);
        logDebug("ForwardModule::KernelMessageWorker::tmain: Using %lx", 
            (unsigned long)kmsg);
        conn = kmsg->getConnection();
        policy = conn->getPolicy();

        switch (kmsg->getType())
        {
        case KernelMessage::Type::DATAIN_ACK:
            policy->acquireOnACK()->operator()(kmsg, interface, policy);
            policy->readRelease();
            conn->renewOnInput(kmsg);
            break;
        case KernelMessage::Type::RETX:
            policy->acquireOnRetx()->operator()(kmsg, interface, policy);
            policy->readRelease();
        case KernelMessage::Type::DATAOUT:
            // This is handled by InterfaceManager, we're not doing this here 
            // due to performance issue.
            // Note: this is essential because KernelMessageProviders are not
            // required to provide message types other than DATAIN.
            //conn->renewOnOutput(kmsg);
            break;
        default:
            logError("ForwardModule::KernelMessageWorker::tmain: "
                "Unknown kernel message type %d", kmsg->getType());
        }
        delete kmsg;
    }
}

}