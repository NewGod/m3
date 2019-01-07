#ifndef __FIXSTACK_HH__
#define __FIXSTACK_HH__

#include "Log.h"
#include "Represent.h"

namespace m3
{

template <typename T>
class FixStack
{
protected:
    int len;
    T *arr;
    T *pos;
public:
    FixStack(): arr(0) {}

    ~FixStack()
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
            logAllocError("FixStack::init");
            return 1;
        }
        this->len = len;
        pos = arr;
        return 0;
    }

    inline bool isEmpty()
    {
        return pos == arr;
    }

    inline bool isFull()
    {
        return pos - arr == len;
    }

    inline int push(T &item)
    {
        *(pos++) = item;
        return 0;
    }

    inline T pop()
    {
        return *(--pos);
    }

    inline T peek()
    {
        return *(pos - 1);
    }
};

template <typename T>
using PointerFixStack = FixStack<T*>;

}

#endif