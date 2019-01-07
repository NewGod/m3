#ifndef __PACKETFILTER_HH__
#define __PACKETFILTER_HH__

#include <netinet/ether.h>
#include <netinet/ip.h>

#include "bpf.h"

#include "Represent.h"

namespace m3
{

class PacketFilter
{
    struct bpf_node *root;
public:
    PacketFilter(const char* filter = nullptr);
    virtual ~PacketFilter();
    virtual int init(const char* filter);
    virtual bool drop(iphdr *pak);
};

}//m3
#endif
