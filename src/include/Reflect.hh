#ifndef __REFLECT_HH__
#define __REFLECT_HH__

#include <atomic>
#include <cstring>

#include "Log.h"
#include "Represent.h"
#include "RWLock.hh"
#include "StringUtils.h"

namespace m3
{

class Reflect
{
public:
    template <class T>
    void *newInstance()
    {
        return T::newInstance();
    }

    typedef void* (*CreateMethod)(void);
protected:
    static StringKeyHashMap<CreateMethod> reflect;
public:
    static CreateMethod getCreateMethod(const char *name)
    {
        auto node = reflect.find(name);
        iferr (node == reflect.end())
        {
            return 0;
        }
        else
        {
            return node->second;
        }
        
    }
    static inline void* create(const char *name)
    {
        CreateMethod method = getCreateMethod(name);

        iferr (method == 0)
        {
            logError("Reflect::create: No constructor for %s found.",
                name);
            return 0;
        }
        else
        {
            return method();
        }
    }
};

}

#endif
