#include "Log.h"
#include "OfflinePCAP.hh"
#include "Represent.h"

namespace m3
{

OfflinePCAP::~OfflinePCAP()
{
    if (dumper)
    {
        pcap_dump_close(dumper);
    }
}

int OfflinePCAP::init(const void *arg)
{
    const InitObject *iarg = (const InitObject*)arg;
    
    iferr ((source = pcap_open_offline(iarg->rpath, errbuf)) == 0)
    {
        logError("OfflinePCAP::init: Can't open input pcap file %s.(%s)", 
            iarg->rpath, errbuf);
        return 1;
    }

    switch (pcap_datalink(source))
    {
    case LINKTYPE_ETHERNET:
        offset = 14;
        break;
    case LINKTYPE_LINUX_SLL:
        offset = 16;
        break;
    default:
		logError("OfflinePCAP::init: Unknown link type (%x).", 
            pcap_datalink(source));
        return 2;
    }

    if (iarg->wpath != 0)
    {
        iferr ((dumper = pcap_dump_open(source, iarg->wpath)) == 0)
        {
            logError(
                "OfflinePCAP::init: Can't open output pcap file %s.(%s)", 
                iarg->wpath, pcap_geterr(source));
            return 3;
        }
    }

    return 0;
}

int OfflinePCAP::writePacket(void *buf, int len)
{
    int ret;
    pcap_pkthdr hdr;

    if (dumper == 0)
    {
        logError("OfflinePCAP::writePacket: initialize with non-empty dump"
            " file name to enable writing.");
        return 1;
    }

    hdr.len = hdr.caplen = len;
    iferr ((ret = gettimeofday(&hdr.ts, 0)) != 0)
    {
        char errbuf[64];
        logError("OfflinePCAP::writePacket: gettimeofday() failed(%s)",
            strerrorV(errno, errbuf));
        return 2;
    }

    pcap_dump((u_char*)dumper, &hdr, (u_char*)buf);

    return 0;
}

}