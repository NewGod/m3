#ifndef __AUTODESTROYABLE_HH__
#define __AUTODESTROYABLE_HH__

#include <atomic>

#include "Log.h"

namespace m3
{

class AutoDestroyable
{
protected:
    std::atomic<int> refCount;
public:
    AutoDestroyable() : refCount(0) {}
    virtual ~AutoDestroyable() {};

    inline int getRefCount()
    {
        return refCount.load();
    }
    inline int reference()
    {
        logDebug("AutoDestroyable::reference: %lx refcount %d", 
            (unsigned long)this, refCount.load() + 1);
        return refCount++ < 0 ? 1 : 0;
    }
    inline void release()
    {
        int expected = 1;

        logDebug("AutoDestroyable::release: %lx refcount %d", 
            (unsigned long)this, refCount.load() - 1);

        if (refCount.compare_exchange_strong(expected, -2147483647))
        {
            delete this;
        }
        else
        {
            refCount--;
        }

    }
};

}

#endif
