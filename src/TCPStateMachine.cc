#include "DataMessageHeader.hh"
#include "IPPacket.hh"
#include "Log.h"
#include "PacketReader.hh"
#include "ProtocolMetric.hh"
#include "TCPOptionWalker.hh"
#include "TCPStateMachine.hh"

namespace m3
{

typedef TCPStateMachine::State State;

const State TCPStateMachine::stateMap[256] = 
{
    // 0
    State::PENDING, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::PENDING, State::PENDING, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::PENDING, State::PENDING, State::CLOSED, State::PENDING, 
    // 16
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    // 32
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    // 48
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    // 64
    State::SYN_SENT, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::SYN_SENT, State::SYN_SENT, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::SYN_RCVD, State::SYN_RCVD, State::CLOSED, State::SYN_RCVD, 
    // 80
    State::SYN_SENT, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::SYN_SENT, State::SYN_SENT, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::FINWAIT_1, State::FINWAIT_1, State::CLOSED, State::CLOSING, 
    // 96
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    // 112
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    // 128
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    // 144
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    // 160
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    // 176
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    // 192
    State::SYN_SENT, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::SYN_SENT, State::SYN_SENT, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::ESTAB, State::ESTAB, State::CLOSED, State::CLOSE_WAIT, 
    // 208
    State::SYN_SENT, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::SYN_SENT, State::SYN_SENT, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::FINWAIT_1, State::FINWAIT_1, State::CLOSED, State::CLOSING, 
    // 224
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    // 240
    State::SYN_SENT, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::SYN_SENT, State::SYN_SENT, State::CLOSED, State::CLOSED, 
    State::CLOSED, State::CLOSED, State::CLOSED, State::CLOSED, 
    State::FINWAIT_2, State::FINWAIT_2, State::CLOSED, State::TIME_WAIT
};

const unsigned char TCPStateMachine::stateInvMap[] =
{
    0, 64, 76, 204, 220, 252, 207, 223, 255, 1
};

// unit: 10s
const unsigned TCPStateMachine::expireValueMap[] =
{
    6, 12, 6, 43200, 12, 12, 12, 12, 2, 1
};

const char* const TCPStateMachine::stateString[] = 
{
    "PENDING",
    "SYN_SENT",
    "SYN_RCVD",
    "ESTAB",
    "FINWAIT_1",
    "FINWAIT_2",
    "CLOSE_WAIT",
    "CLOSING",
    "TIME_WAIT",
    "CLOSED"
};

void TCPStateMachine::transformOnInput(Message *msg)
{
    IPPacket *pak = msg->toIPPacket();
    DataMessageHeader *dh = (DataMessageHeader*)pak->dataHeader;
    tcphdr *tcph = &dh->tcph;
    unsigned char oldState;
    unsigned seq = ntohl(tcph->seq);

    lock.writeLock();

    oldState = state;

    logDebug("TCPStateMachine::transformOnInput: Current %s(0x%02x)",
        stateString[stateMap[oldState]], oldState);

    if (tcph->rst)
    {
        state = stateInvMap[State::CLOSED];
        goto trOutput_out;
    }

    if (tcph->syn)
    {
        rsynDone = 1;
        rsynseq = seq;
        logDebug("TCPStateMachine::transformOnInput: RSYN %u", seq);
    }
    if (tcph->fin)
    {
        rfinDone = 1;
        rfinseq = seq;
        logDebug("TCPStateMachine::transformOnInput: RFIN %u", seq);
    }
    if (tcph->ack)
    {
        unsigned ackseq = ntohl(tcph->ack_seq);
        if (synDone && !synACKed && isACKed(synseq, ackseq))
        {
            logDebug("TCPStateMachine::transformOnInput: SYN ACKed");
            synACKed = 1;
        }
        if (finDone && !finACKed && isACKed(finseq, ackseq))
        {
            logDebug("TCPStateMachine::transformOnInput: FIN ACKed");
            finACKed = 1;
        }
    }

trOutput_out:
    if (oldState != state)
    {
        logMessage("TCPStateMachine::transformOnInput: To %s(0x%02x)",
            stateString[stateMap[state]], state);
        PacketReader::m3Decode(dh);
    }
    expire = expireValueMap[stateMap[state]];
    lock.writeRelease();
}

void TCPStateMachine::transformOnOutput(Message *msg)
{
    IPPacket *pak = msg->toIPPacket();
    DataMessageHeader *dh = (DataMessageHeader*)pak->dataHeader;
    tcphdr *tcph = &dh->tcph;
    unsigned char oldState;
    unsigned seq = ntohl(tcph->seq);

    lock.writeLock();

    oldState = state;

    logDebug("TCPStateMachine::transformOnOutput: Current %s(0x%02x)",
        stateString[stateMap[oldState]], oldState);

    if (tcph->rst)
    {
        state = stateInvMap[State::CLOSED];
        goto trOutput_out;
    }

    if (tcph->syn)
    {
        synDone = 1;
        synseq = seq;
        logDebug("TCPStateMachine::transformOnOutput: SYN %u", seq);
    }
    if (tcph->fin)
    {
        finDone = 1;
        finseq = seq;
        logDebug("TCPStateMachine::transformOnOutput: FIN %u", seq);
    }
    if (tcph->ack)
    {
        unsigned ackseq = ntohl(tcph->ack_seq);
        if (rsynDone && !rsynACKed && isACKed(rsynseq, ackseq))
        {
            logDebug("TCPStateMachine::transformOnOutput: RSYN ACKed");
            rsynACKed = 1;
        }
        if (rfinDone && !rfinACKed && isACKed(rfinseq, ackseq))
        {
            logDebug("TCPStateMachine::transformOnOutput: RFIN ACKed");
            rfinACKed = 1;
        }
    }

trOutput_out:
    if (oldState != state)
    {
        logMessage("TCPStateMachine::transformOnOutput: To %s(0x%02x)",
            stateString[stateMap[state]], state);
        PacketReader::m3Decode(dh);
    }
    expire = expireValueMap[stateMap[state]];
    lock.writeRelease();
}

}