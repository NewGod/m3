#include "Connection.hh"
#include "ConnectionManager.hh"
#include "Context.hh"
#include "ContextManager.hh"
#include "InternalMessage.hh"

namespace m3
{

ContextManager *ContextManager::instance = new ContextManager();

int ContextManager::getContext(Type type, Context **dest)
{
    int res = 0;
    lock.readLock();

    auto ite = contextMap.find(type);
    if (ite == contextMap.end())
    {
        logWarning("ContextManager::getContext: Unrecognized context type %d", 
            type);
        res = 1;
    }
    else
    {
        *dest = ite->second;
    }

    lock.readRelease();
    return res;
}

int ContextManager::addContext(Context *src)
{
    lock.writeLock();
    contextMap[src->getType()] = src;
    lock.writeRelease();
    return 0;
}

}