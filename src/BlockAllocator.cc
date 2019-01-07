#include "Log.h"
#include "BlockAllocator.hh"

namespace m3
{

void *BlockAllocator::allocate()
{
    void *res = 0;
    int i;
    
    ifsucc (!avail.isEmpty())
    {
        i = avail.pop();
        logDebug("ALLOC %d", i);
        res = pool + (i * bs);
        iferr (refcnt[i] != 0)
        {
            logError("BlockAllocator::allocate: Alloc error %d %d", 
                i, refcnt[i]);
        }
        else
        {
            refcnt[i] = 1;
        }
    }
    else
    {
        logError("BlockAllocator::allocate: Out of memory.");
    }

    return res;
}

void BlockAllocator::release(const void *ptr)
{
    int num = ((char*)ptr - pool) / bs;
    logDebug("RELSE %d", num);
    if (--refcnt[num] == 0)
    {
        avail.push(num);
    }
}

void BlockAllocator::reference(const void *ptr)
{
    int num = ((char*)ptr - pool) / bs;
    refcnt[num]++;
}

int BlockAllocator::init(void *arg)
{
    int ret;

    InitObject *iobj = (InitObject*)arg;

    if (this->bs == ((iobj->bs + 7) & ~7) && this->cnt == iobj->count)
    {
        return 0;
    }

    this->bs = ((iobj->bs + 7) & ~7);
    this->cnt = iobj->count;

    logDebug("BS %d, cnt %d", this->bs, this->cnt);

    iferr ((pool = snew char[this->bs * this->cnt]) == 0)
    {
        logAllocError("BlockAllocator::init");
        return 1;
    }

    logDebug("pool %lx", (unsigned long)pool);

    iferr ((ret = avail.init(this->cnt)) != 0)
    {
        logStackTrace("BlockAllocator::init", ret);
        return 2;
    }

    iferr ((refcnt = snew int[this->cnt]) == 0)
    {
        logAllocError("BlockAllocator::init");
        return 1;
    }

    for (int i = 0; i < this->cnt; ++i)
    {
        avail.push(i);
        refcnt[i] = 0;
    }

    return 0;
}

}
