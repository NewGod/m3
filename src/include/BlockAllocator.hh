#ifndef __BLOCKALLOCATOR_HH__
#define __BLOCKALLOCATOR_HH__

#include "FixQueue.hh"
#include "FixStack.hh"
#include "Represent.h"
#include "RWLock.hh"

namespace m3
{

class BlockAllocator
{
protected:
    int cnt;
    int bs;
    FixStack<int> avail;
    int *refcnt;
    char *pool;
public:
    struct InitObject
    {
        int bs;
        int count;

        InitObject()
        {
            memset(this, 0, sizeof(InitObject));
        }
    };
    BlockAllocator(): cnt(0), bs(0), avail(), refcnt(), pool(0) {}

    virtual void *allocate();
    virtual void release(const void *ptr);
    virtual void reference(const void *ptr);
    virtual int init(void *arg);
    inline int getBlockSize()
    {
        return bs;
    }
};

class ConcurrentBlockAllocator : public BlockAllocator
{
protected:
    RWLock lock;
public:
    ConcurrentBlockAllocator(): lock() {}

    void *allocate() override
    {
        void* res;
        lock.writeLock();
        res = BlockAllocator::allocate();
        lock.writeRelease();
        return res;
    }
    void release(const void *ptr) override
    {
        lock.writeLock();
        BlockAllocator::release(ptr);
        lock.writeRelease();
    }
    void reference(const void *ptr) override
    {
        lock.writeLock();
        BlockAllocator::reference(ptr);
        lock.writeRelease();
    }
    int init(void *arg) override
    {
        int res;
        lock.writeLock();
        res = BlockAllocator::init(arg);
        lock.writeRelease();
        return res;
    }
};

}

#endif
