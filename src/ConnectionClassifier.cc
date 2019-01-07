#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "Connection.hh"
#include "ConnectionClassifier.hh"
#include "DataMessage.hh"
#include "PacketReader.hh"
#include "TypeDetector.hh"
#include "ThreadUtils.hh"

namespace m3
{

int ConnectionClassifier::init(PacketSource *source, ConnectionManager *connmgr,
    PolicyManager *plcmgr)
{
    this->source = source;
    this->connmgr = connmgr;
    this->plcmgr = plcmgr;
    
    if ((cw = snew ClassifyWorker(this, source, connmgr, plcmgr)) == 0)
    {
        logAllocError("ConnectionClassifier::init");
        return 1;
    }

    memset(detectors, 0, sizeof(detectors));

    return 0;
}

void ConnectionClassifier::detect(DataMessage *dmsg)
{
    ConnectionType *type = dmsg->getConnection()->getType();
    TypeDetector *det;
    if (type == 0 || (det = type->getDetector()) == 0)
    {
        for (int i = 0; i < MAX_DETECTOR; ++i)
        {
            if (detectors[i] != 0)
            {
                if (detectors[i]->detectFrom(dmsg) != 0)
                {
                    break;
                }
            }
        }
    }
    else
    {
        det->detectFrom(dmsg);
    }
}

void ConnectionClassifier::ClassifyWorker::tmain(ConnectionClassifier *clasf, 
    PacketSource *source, ConnectionManager *connmgr, PolicyManager *plcmgr)
{
    IPPacket pak;
    Connection *conn;
    DataMessage *msg;
    DataMessageHeader *dh;

    logMessage("ConnectionClassifier::ClassifyWorker::tmain: Thread identify");

#ifdef ENABLE_FORCE_THREAD_SCHEDULE
    SetRealtime();
    AssignCPU();
#endif

    while (1)
    {
        int ret;
        iferr ((ret = source->next(&pak)) != 0)
        {
            logStackTrace("ConnectionClassifier::ClassifyWorker::tmain", ret);
            logStackTraceEnd();
            continue;
        }

        //logMessage(
        //    "ConnectionClassifier::ClassifyWorker::tmain: Message constructing.");

        iferr ((msg = snew DataMessage(&pak)) == 0)
        {
            logAllocError("ConnectionClassifier::ClassifyWorker::tmain");
            continue;
        }
        
        dh = msg->getHeader();
        // Message constructed here is userdata.
        dh->type = DataMessageHeader::Type::USERDATA;
        dh->subtype = DataMessageHeader::UserDataSubtype::UD_DATA;
        
        // get connection information first
        iferr ((ret = connmgr->getConnection(msg)) != 0)
        {
            logStackTrace("ConnectionClassifier::ClassifyWorker::tmain", ret);
            logStackTraceEnd();
            PacketReader::packetDecode(pak.getIPHeader(), pak.getIPLen());
            PacketReader::m3Decode((DataMessageHeader*)pak.dataHeader);
            delete msg;
            continue;
        }

        conn = msg->getConnection();

        // classify
        if (dh->getTCPLen() != 0)
        {
            clasf->detect(msg);
        }

        // modify policy and insert packet to connection buffer
        iferr ((ret = plcmgr->modifyPolicy(conn, msg)) != 0)
        {
            logStackTrace("ConnectionClassifier::ClassifyWorker::tmain", ret);
            logStackTraceEnd();
            PacketReader::packetDecode(pak.getIPHeader(), pak.getIPLen());
            PacketReader::m3Decode((DataMessageHeader*)pak.dataHeader);
            delete msg;
            continue;
        }
        iferr ((ret = connmgr->issue(msg)) != 0)
        {
            logStackTrace("ConnectionClassifier::ClassifyWorker::tmain", ret);
            logStackTraceEnd();
            PacketReader::packetDecode(pak.getIPHeader(), pak.getIPLen());
            PacketReader::m3Decode((DataMessageHeader*)pak.dataHeader);
            delete msg;
        }
        //logMessage(
        //    "ConnectionClassifier::ClassifyWorker::tmain: Message issued.");
    }
}

void ConnectionClassifier::addDetector(TypeDetector *det)
{
    detectors[det->getType()] = det;
}

void ConnectionClassifier::removeDetector(TypeDetector *det)
{
    detectors[det->getType()] = 0;
}

}