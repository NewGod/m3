#ifndef __TCPCALLBACK_HH__
#define __TCPCALLBACK_HH__

namespace m3
{

class Interface;
class KernelMessage;
class Policy;

class TCPCallback
{
public:
    virtual ~TCPCallback() {}
    virtual int operator()(KernelMessage *kmsg, 
        Interface *interface, Policy *policy) = 0;
};

}

#endif
