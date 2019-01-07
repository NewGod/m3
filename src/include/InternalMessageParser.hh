#ifndef __INTERNALMESSAGEPARSER_HH__
#define __INTERNALMESSAGEPARSER_HH__

#include "DataMessage.hh"
#include "DataMessageHeader.hh"

namespace m3
{

class InternalMessageContext;

class AbstractInternalMessageParser
{
public:
    virtual ~AbstractInternalMessageParser() {}
    virtual int parse(DataMessage *msg) = 0;
};

class DumbInternalMessageParser : public AbstractInternalMessageParser
{
public:
    static void* newInstance() //@ReflectDumbInternalMessageParser
    {
        return snew DumbInternalMessageParser();
    }
    virtual int parse(DataMessage *msg) override
    {
        return 1;
    }
};

class InternalMessageParser : public AbstractInternalMessageParser
{
protected:
    InternalMessageContext *coxt;
    static int initContext();
public:
    InternalMessageParser();
    static void* newInstance() //@ReflectInternalMessageParser
    {
        return snew InternalMessageParser();
    }
    inline bool isInternalMessage(DataMessage *msg)
    {
        return msg->getHeader()->type == DataMessageHeader::Type::INTERNAL;
    }
    virtual int parse(DataMessage *msg) override;
};

}

#endif

