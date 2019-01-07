#ifndef __BLOCKQUEUEBUFFER_HH__
#define __BLOCKQUEUEBUFFER_HH__

#include <atomic>

#include "Represent.h"
#include "Semaphore.hh"

namespace m3
{

template <typename T>
class BlockQueueBuffer
{
protected:
    Semaphore slot;
    Semaphore item;
    std::atomic<std::uint32_t> writePos;
    std::atomic<std::uint32_t> readPos;
    T *arr;
    uint32_t len;
    uint32_t mask;
public:
    static const int MAXLEVEL = 30;
    static const int DEFLEVEL = 12;
    inline BlockQueueBuffer(int level = DEFLEVEL): slot(1 << level), item()
    {
        len = 1 << level;
        arr = snew T[len];
        mask = len - 1;
        writePos = 0;
        readPos = 0;
    }
    virtual ~BlockQueueBuffer()
    {
        delete[] arr;
    }

    inline int getlen()
    {
        return len;
    }
    inline void advance(T* ptr)
    {
        slot.issue();
        uint32_t myPos = (writePos++) & mask;
        arr[myPos] = *ptr;
        item.release();
    }
    inline void purchase(T* dest)
    {
        item.issue();
        uint32_t myPos = (readPos++) & mask;
        *dest = arr[myPos];
        slot.release();
    }
};

template <typename T>
using BlockPointerQueueBuffer = BlockQueueBuffer<T*>;

}

#endif
