#ifndef __LIVEPCAP_HH__
#define __LIVEPCAP_HH__

#include "PCAP.hh"

namespace m3
{

class LivePCAP : public PCAP
{
public:
    struct InitObject
    {
        const char *device;
        int snaplen;
    };

    virtual ~LivePCAP() {}
    LivePCAP(): PCAP() {}
    int init(const void *arg) override;
};
 
}

#endif
