#ifndef __TCPSTATEMACHINE_HH__
#define __TCPSTATEMACHINE_HH__

#include <atomic>
#include <set>

#include "Message.hh"
#include "Represent.h"
#include "RWLock.hh"
#include "TCPOptionWalker.hh"

namespace m3
{

class TCPStateMachine
{
public:
    enum State { PENDING = 0, SYN_SENT, SYN_RCVD, ESTAB, FINWAIT_1, 
        FINWAIT_2, CLOSE_WAIT, CLOSING, TIME_WAIT, CLOSED, count };
    static const unsigned char stateInvMap[];
    static const State stateMap[256];
    static const char* const stateString[];
    static const unsigned expireValueMap[];
protected:
    class Interval
    {
    public:
        unsigned left;
        unsigned right;
        inline bool operator==(const Interval& rv)
        {
            return rv.left == left && rv.right == right;
        }
        inline bool operator<(const Interval& rv)
        {
            return left < rv.left ? 
                1 : (left == rv.left ? right < rv.right : 0);
        }
    };

    unsigned synseq;
    unsigned finseq;
    unsigned rsynseq;
    unsigned rfinseq;
    union
    {
        struct
        {
            unsigned char rfinDone:1;
            unsigned char rfinACKed:1;
            unsigned char rsynDone:1;
            unsigned char rsynACKed:1;
            unsigned char finDone:1;
            unsigned char finACKed:1;
            unsigned char synDone:1;
            unsigned char synACKed:1;
        };
        unsigned char state;
    };

    unsigned expire;
    
    static inline bool isACKed(unsigned seq, unsigned ack)
    {
        return (int)(ack - seq - 1) >= 0;
    }

    RWLock lock;
public:
    inline State getState()
    {
        State res;
        lock.readLock();
        res = stateMap[state];
        lock.readRelease();
        return res;
    }
    inline void setState(unsigned char state)
    {
        lock.writeLock();
        this->state = state;
        this->expire = expireValueMap[stateMap[state]];
        lock.writeRelease();
    }
    static inline const char* getStateString(State state)
    {
        return stateString[state];
    }
    void transformOnInput(Message *msg);
    void transformOnOutput(Message *msg);
    inline unsigned reduceExpire()
    {
        unsigned res;
        lock.writeLock();
        res = --expire;
        lock.writeRelease();
        return res;
    }
    inline unsigned getExpire()
    {
        unsigned res;
        lock.readLock();
        res = expire;
        lock.readRelease();
        return res;
    }
    static void* newInstance() //@ReflectTCPStateMachine
    {
        return snew TCPStateMachine();
    }
    TCPStateMachine(): state(0), lock() {}
};

}

#endif
