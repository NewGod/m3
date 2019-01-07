#ifndef __DUMBTCPCALLBACK_HH__
#define __DUMBTCPCALLBACK_HH__

#include "TCPCallback.hh"

namespace m3
{

class DumbTCPCallback : public TCPCallback
{
public:
    static DumbTCPCallback instance;
    int operator()(KernelMessage *kmsg, 
        Interface *interface, Policy *policy) override
    {
        return 0;
    }
    // don't really create a new instance
    static void* newInstance() //@ReflectDumbTCPCallback
    {
        return &instance;
    }
};

}

#endif
