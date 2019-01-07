#ifndef __KERNELMESSAGEPROVIDER_HH__
#define __KERNELMESSAGEPROVIDER_HH__

#include <netinet/ip.h>
#include <pcap/pcap.h>

namespace m3
{

class KernelMessage;
class MetricProvider;

// A KernelMessageProvider is required to provide DATAIN messages only. That is
// to say, if you are using policies with onRetx or onACK callbacks, you should 
// ensure the KernelMessageProvider you are working with supports corresponding 
// message types.
class KernelMessageProvider
{
protected:
    MetricProvider *mp;
public:
    virtual int next(KernelMessage **dest) = 0;
    virtual int init(void *arg) = 0;
    inline MetricProvider *getProvider()
    {
        return mp;
    }
};

}

#endif
