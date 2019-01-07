#ifndef __SPINQUEUEBUFFER_HH__
#define __SPINQUEUEBUFFER_HH__

#include <atomic>
#include <cstdint>

#include "Log.h"
#include "Represent.h"

namespace m3
{

template <typename T>
class SpinQueueBuffer
{
protected:
    inline bool hasNext(uint32_t myPurchase)
    {
        return myPurchase != head.load();
    }
    void doAdvance(uint32_t myClaim, T* item)
    {
        // we don't need this here due to a true-dependency.
        // std::atomic_signal_fence(std::memory_order_seq_cst);
        arr[myClaim & mask] = *item;
        while (head.load() != myClaim);
        std::atomic_signal_fence(std::memory_order_seq_cst);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        head = myClaim + 1;
    }
    void doPurchase(uint32_t myPurchase, T* dest)
    {
        *dest = arr[myPurchase & mask];
        while (tail.load() != myPurchase);
        // ensure that we have done writing before update read pointer
        std::atomic_signal_fence(std::memory_order_seq_cst);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        tail = myPurchase + 1;
    }

    T *arr;
    uint32_t len;
    uint32_t mask;
    std::atomic<std::uint32_t> peek;
    std::atomic<std::uint32_t> tail;
    std::atomic<std::uint32_t> head;
    std::atomic<std::uint32_t> claim;
public:
    static const int MAXLEVEL = 30;
    static const int DEFLEVEL = 12;

    inline SpinQueueBuffer(int level = DEFLEVEL)
    {
        len = 1 << level;
        arr = snew T[len];
        mask = len - 1;
        head = 0;
        tail = 0;
        claim = 0;
        peek = 0;
    }
    virtual ~SpinQueueBuffer()
    {
        delete[] arr;
    }
    inline int getlen()
    {
        return len;
    }
    inline int tryAdvance(T* item)
    {
        uint32_t myClaim = claim++;
        if ((int)(myClaim - tail.load()) >= (int)len)
        {
            claim--;
            return -1;
        }

        doAdvance(myClaim, item);
        return 0;
    }
#ifdef SPINQUEUEBUFFER_DEBUG
    inline uint32_t advance(T* item)
#else
    inline void advance(T* item)
#endif
    {
        uint32_t myClaim = claim++;
        while ((int)(myClaim - tail.load()) >= (int)len);
        doAdvance(myClaim, item);

#ifdef SPINQUEUEBUFFER_DEBUG
        return myClaim;
#endif
    }
    inline int tryPurchase(T* dest)
    {
        uint32_t myPurchase = peek++;
        if (!hasNext(myPurchase))
        {
            peek--;
            return -1;
        }
        
        doPurchase(myPurchase, dest);
        return 0;
    }

#ifdef SPINQUEUEBUFFER_DEBUG
    inline uint32_t purchase(T* dest)
#else
    inline void purchase(T* dest)
#endif
    {
        uint32_t myPurchase = peek++;
        while (!hasNext(myPurchase));

        doPurchase(myPurchase, dest);
        
#ifdef SPINQUEUEBUFFER_DEBUG
        return myPurchase;
#endif
    }

    inline bool hasNext()
    {
        return hasNext(peek);
    }
};

template <typename T>
using SpinPointerQueueBuffer = SpinQueueBuffer<T*>;

}

#endif
