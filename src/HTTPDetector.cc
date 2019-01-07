#include "Connection.hh"
#include "DataMessage.hh"
#include "HTTPDetector.hh"
#include "IPPacket.hh"
#include <typeinfo>

namespace m3
{

// we parse GET requests here only
// we assume most requests start at the first byte of a packet
int HTTPDetector::getRequest(char *str, HTTPRequest *req)
{
    if (!HTTPRequest::isRequest(str))
    {
        return 1;
    }

    logDebug("Req %s", str);

    iferr (req && req->init(str) != 0)
    {
        return 2;
    }

    return 0;
}

int HTTPDetector::getResponse(char *str, HTTPResponse *resp)
{
    if (!HTTPResponse::isResponse(str))
    {
        return 1;
    }

    logDebug("Rep %s", str);

    iferr (resp && resp->init(str) != 0)
    {
        return 2;
    }

    return 0;
}

// HTTP ConnectionTypes should return this detector when getDetector() is called
ConnectionType::Type HTTPDetector::setTypeForResponse(
    DataMessage *msg, HTTPResponse *resp)
{
    ConnectionType *type = msg->getConnection()->getType();
    // if(!type)
    // {
    //     // do something
    // }
    // else
    // {
    //     // do something
    // }
    if(!type)
    {
        type = &dumb;
    }

    iferr (type == 0)
    {
        logAllocError("HTTPDetector::setTypeForResponse");
        return ConnectionType::Type::NOTYPE;
    }
    msg->getConnection()->setType(type);
    return type->getType();
}

ConnectionType::Type HTTPDetector::detectFrom(DataMessage *msg)
{
    IPPacket *pak = msg->toIPPacket();
    Connection *conn = msg->getConnection();
    iphdr *iph = pak->getIPHeader();
    tcphdr *tcph = pak->getTCPHeader(iph);
    char *buf = pak->getTCPData(tcph);
    int ret;

    // from server
    if (conn->getSource()->sin_addr.s_addr != iph->saddr)
    {
        HTTPResponse resp;

        if (!conn->getType() || conn->getType()->getDetector() != this)
        {
            return ConnectionType::Type::NOTYPE;
        }

        if ((ret = getResponse(buf, &resp)) == 0)
        {
            return setTypeForResponse(msg, &resp);
        }

        return ConnectionType::Type::NOTYPE;
    }
    // from client
    else
    {
        ConnectionMetadata *meta = (ConnectionMetadata*)conn->getMeta();
        ConnectionMetadataValue *metaval = (ConnectionMetadataValue*)meta->acquireValue();
        unsigned long pakSent;
        if(!metaval)
            pakSent = 0;
        else
            pakSent = metaval->pakSent;
        meta->releaseValue();

        if (pakSent != 0)
        {
            return ConnectionType::Type::NOTYPE;
        }

        if ((ret = getRequest(buf, 0)) == 0)
        {
            conn->setType(&dumb);
            return ConnectionType::Type::HTTP_GENERIC;
        }

        return ConnectionType::Type::NOTYPE;
    }

    // make g++ happy...
    return ConnectionType::Type::NOTYPE;
}

}