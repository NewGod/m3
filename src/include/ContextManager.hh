#ifndef __CONTEXTMANAGER_HH__
#define __CONTEXTMANAGER_HH__

#include "Context.hh"
#include "RWLock.hh"
#include "StringUtils.h"

namespace m3
{

class Connection;
class ConnectionManager;

class ContextManager
{
protected:
    static ContextManager *instance;

    std::unordered_map<int, Context*> contextMap;
    RWLock lock;
public:
    typedef Context::Type Type;

    ContextManager(): contextMap(), lock() {}
    ~ContextManager() {}
    static void* newInstance() //@ReflectContextManager
    {
        return instance;
    }
    static ContextManager *getInstance()
    {
        return instance;
    }
    int getContext(Context::Type type, Context **dest);
    int addContext(Context *src);
};

}

#endif
