#include "Connection.hh"
#include "ConnectionManager.hh"
#include "ContextManager.hh"
#include "DataMessageHeader.hh"
#include "InternalMessage.hh"
#include "Log.h"
#include "Policy.hh"
#include "PolicyManager.hh"

namespace m3
{

Connection *InternalMessage::conn = 0;

InternalMessage::InternalMessage(void *buf, int len, 
    DataMessageMetaHeader::InternalSubtype subtype):
    DataMessage()
{
    DataMessageMetaHeader *ih = (DataMessageMetaHeader*)buf - 1;

    ih->tlen = sizeof(DataMessageMetaHeader) + len;
    ih->generateChk();
    ih->type = DataMessageMetaHeader::Type::INTERNAL;
    ih->subtype = subtype;

    tofree = true;
    pak.header = pak.dataHeader = (char*)ih;
    pak.len = ih->tlen;

    setConnection(conn);
}

int InternalMessage::initInternalMessage(ConnectionManager *connmgr,
    PolicyManager *plcmgr)
{
    DataMessageHeader dh;
    dh.type = DataMessageMetaHeader::Type::INTERNAL;
    iferr ((conn = snew StatelessConnection(&dh)) == 0)
    {
        logAllocError("InternalMessage::initInternalMessage");
        return 1;
    }

    conn->reference();
    conn->getStateMachine()->setState(TCPStateMachine::stateInvMap[
        TCPStateMachine::State::ESTAB]);
    plcmgr->assignDefaultPolicy(conn, Policy::Priority::REALTIME);
    // conn->getPolicy()->setPrio(Policy::Priority::REALTIME);
    connmgr->addConnectionNoTrack(conn);

    return 0;
}

}