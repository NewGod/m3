#ifndef __FIXQUEUE_HH__
#define __FIXQUEUE_HH__

#include "Log.h"
#include "Represent.h"

namespace m3
{

template <typename T>
class FixQueue
{
protected:
    int len;
    int cnt;
    T *arr;
    int head;
    int tail;
public:
    FixQueue(): arr(0) {}

    ~FixQueue()
    {
        if (arr)
        {
            delete arr;
        }
    }

    int init(int len)
    {
        if (arr)
        {
            delete arr;
        }

        iferr ((arr = snew T[len]) == 0)
        {
            logAllocError("FixQueue::init");
            return 1;
        }
        this->len = len;
        cnt = 0;
        head = tail = 0;
        return 0;
    }

    inline bool isEmpty()
    {
        return cnt == 0;
    }

    inline bool isFull()
    {
        return cnt == len;
    }

    inline int push(T &item)
    {
        arr[head++] = item;
        if (head == len)
        {
            head = 0;
        }
        cnt++;
        return 0;
    }

    inline T pop()
    {
        T* res = arr + tail++;
        if (tail == len)
        {
            tail = 0;
        }
        cnt--;
        return *res;
    }
    
    inline void erase()
    {
        if (++tail == len)
        {
            tail = 0;
        }
        cnt--;
    }

    inline T& peek()
    {
        return arr[tail];
    }

    inline int size()
    {
        return cnt;
    }
};

template <typename T>
using PointerFixQueue = FixQueue<T*>;

}

#endif