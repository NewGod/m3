#ifndef __CONCURRENTVALUEHOLDER_HH__
#define __CONCURRENTVALUEHOLDER_HH__

#include "RWLock.hh"

namespace m3
{

class ConcurrentValueHolder
{
protected:
    RWLock lock;
public:
    ConcurrentValueHolder(): lock() {}
    virtual ~ConcurrentValueHolder() {};
    
    virtual const void* getValue() = 0;
    inline const void* acquireValue()
    {
        lock.readLock();
        return getValue();
    }
    inline void releaseValue()
    {
        lock.readRelease();
    }
};

}

#endif
