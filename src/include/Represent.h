#ifndef __REPRESENT_H__
#define __REPRESENT_H__

#include <stdint.h>
#include <stdlib.h>

#include <new>

#define offsetOf(field, type) ((int64_t)&(((type*)0)->field))

#define containerOf(ptr, field, type)\
    ((type*)((int64_t)(ptr) - offsetOf(field, type)))

#define nalloc(type, len)\
    ((type*)malloc(sizeof(type) * (len)))

#define alloc(type) nalloc(type, 1)

#define snew new(std::nothrow)

#define likely(x) __builtin_expect(!!(x), 1)

#define unlikely(x) __builtin_expect(!!(x), 0)

#define iferr(condition) if (unlikely(condition))

#define ifsucc(condition) if (likely(condition))

#define clearptr(ptr)\
    do\
    {\
        if (ptr)\
        {\
            delete ptr;\
        }\
    }\
    while (0)

template<typename T>
struct PointerLess
{
    bool operator()(const T* const& p1, const T* const& p2) const
    {
        return *p1 < *p2;
    }
};

template<typename T>
struct PointerEqual
{
    bool operator()(const T* const& p1, const T* const& p2) const
    {
        return *p1 == *p2;
    }
};

#endif

