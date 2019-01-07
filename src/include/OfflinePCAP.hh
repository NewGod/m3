#ifndef __OFFLINEPCAP_HH__
#define __OFFLINEPCAP_HH__

#include "PCAP.hh"

namespace m3
{

class OfflinePCAP : public PCAP
{
public:
    struct InitObject
    {
        const char *rpath;
        const char *wpath;

        InitObject()
        {
            memset(this, 0, sizeof(InitObject));
        }
    };

    pcap_dumper_t *dumper;

    virtual ~OfflinePCAP();
    OfflinePCAP(): PCAP(), dumper(0) {}
    int init(const void *arg) override;

    // Note: return value 0 don't necessarily mean this call was
    // succeeded, for pcap_dump() doesn't return anything.
    int writePacket(void *buf, int len);
};

}

#endif
