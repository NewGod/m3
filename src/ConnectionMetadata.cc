#include "ConnectionMetadata.hh"
#include "DataMessageHeader.hh"
#include "NetworkUtils.h"
#include "Session.hh"
#include "TCPBasicMetric.hh"

namespace m3
{

long ConnectionMetadataBase::seed = (long)(getTimestamp() * 1000000);

void ConnectionMetadataBase::hash()
{
    unsigned long key[2];

    *key = (unsigned long)src.sin_addr.s_addr *
        (unsigned long)dst.sin_addr.s_addr;
    *(unsigned*)((char*)key + 8) = (unsigned)src.sin_port * 
        (unsigned)dst.sin_port;
    *(unsigned*)((char*)key + 12) = src.sin_addr.s_addr ^
        dst.sin_addr.s_addr;

    this->hashValue = MurmurHash64A(key, sizeof(key), seed);

    logDebug("ConnectionMetadataBase::hash: %lx -> %lu", (unsigned long)this, 
        this->hashValue);
}

int ConnectionMetadataBase::init(void *arg)
{
    DataMessageHeader *dh = (DataMessageHeader*)arg;
    switch (dh->type)
    {
    case DataMessageMetaHeader::Type::USERDATA:
        this->src.sin_family = AF_INET;
        this->src.sin_port = dh->sport;
        this->src.sin_addr = *(in_addr*)&dh->saddr;
        memset(this->src.sin_zero, 0, sizeof(this->src.sin_zero));
        
        this->dst.sin_family = AF_INET;
        this->dst.sin_port = dh->dport;
        this->dst.sin_addr = *(in_addr*)&dh->daddr;
        memset(this->dst.sin_zero, 0, sizeof(this->dst.sin_zero));
        break;
    case DataMessageMetaHeader::Type::INTERNAL:
        memset(&this->src, 0, sizeof(this->src));
        memset(&this->dst, 0, sizeof(this->dst));
        break;
    default:
        memset(&this->src, 0xFF, sizeof(this->src));
        memset(&this->dst, 0xFF, sizeof(this->dst));
        break;
    }
    
    hash();

    return 0;
}

void ConnectionMetadata::renewOnInput(Message *msg)
{
    DataMessageHeader *dh = (DataMessageHeader*)(msg->toIPPacket()->dataHeader);

    iferr (!timeValid)
    {
        timeValid = true;
        TCPBasicMetric::gettime(&startTime);
    }

    lock.writeLock();
    bytesReceived += dh->tlen;
    pakReceived += dh->getTCPLen() != 0;
    lock.writeRelease();
}

void ConnectionMetadata::renewOnOutput(Message *msg)
{
    DataMessageHeader *dh = (DataMessageHeader*)(msg->toIPPacket()->dataHeader);

    iferr (!timeValid)
    {
        timeValid = true;
        TCPBasicMetric::gettime(&startTime);
    }

    lock.writeLock();
    bytesSent += dh->tlen;
    pakSent += dh->getTCPLen() != 0;
    lock.writeRelease();
}

int ConnectionMetadata::init(void *arg)
{
    int ret;
    ConnectionMetadataBase::init(arg);
    iferr ((ret = Session::getSession((DataMessageHeader*)arg, &session)) != 0)
    {
        logStackTrace("ConnectionMetadata::init", ret);
        return 1;
    }
    iferr (ret = session->reference() != 0)
    {
        logStackTrace("ConnectionMetadata::init", ret);
        return 2;
    }
    return 0;
}

ConnectionMetadata::~ConnectionMetadata()
{
    if (session)
    {
        session->release();
    }
}

}
