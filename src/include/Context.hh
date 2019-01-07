#ifndef __CONTEXT_HH__
#define __CONTEXT_HH__

#include "RWLock.hh"

namespace m3
{

class Context
{
public:
    enum Type { INTERNAL, ASYNTCP, count };

    Context() {}
    virtual ~Context() {}
    virtual Type getType() = 0;
};

}

#endif
