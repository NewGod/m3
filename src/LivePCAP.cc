#include "IPPacket.hh"
#include "Log.h"
#include "LivePCAP.hh"

namespace m3
{

// some code from tapo for initailizing a pcap handler...
int LivePCAP::init(const void *arg)
{
    InitObject *iobj = (InitObject*)arg;
    int ret;
    const char *device = iobj->device;
    
    errbuf[0] = 0;
    source = pcap_open_live(device, iobj->snaplen, 1, 1, errbuf);
    if (errbuf[0] != 0)
    {
        logWarning("LivePCAP::init: pcap_open_live error message(%s)",
            errbuf);
    }
    iferr (source == 0)
    {
        logError("LivePCAP::init: pcap_open_live can't open device %s.",
            device);
        return 1;
    }
    //iferr ((ret = pcap_set_immediate_mode(source, 1)) != 0)
    //{
    //    logWarning("LivePCAP::init: pcap_set_immediate_mode failed.");
    //}
    // pcap_set_immediate_mode is not used here for performance considerations.
    
    iferr ((ret = this->me.setMetadata(device)) != 0)
    {
        logStackTrace("LivePCAP::init", ret);
        return 2;
    }
    
    iferr ((ret = pcap_datalink(source)) != LINKTYPE_ETHERNET)
    {
        logWarning("LivePCAP::init: Unrecognized linktype %d, "
            "Only ethernet device is supported.", ret);
        return 3;
    }
    offset = IPPacket::ETH_OFFSET;

    return 0;
}

}