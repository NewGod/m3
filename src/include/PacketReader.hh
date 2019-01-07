#ifndef __PACKETREADER_HH__
#define __PACKETREADER_HH__

#include <linux/if.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/ether.h>
#include <sys/time.h>
#include <pcap/pcap.h>

#include <map>

#include "FixQueue.hh"

namespace m3
{

class DataMessageHeader;
class DeviceHandle;
class Interface;
class IPPacket;
class PacketFilter;

class PacketReader
{
protected:
    static const int MAX_IP_PACKET = 65536;

    DeviceHandle *handle;
    int mss;
    int headerOffset;
    timespec lastCapture;

    FixQueue<u_char*> tsobuf;
    FixQueue<unsigned short> tsolen;

    PacketFilter *filter;

    int nextPacketWithTSO(u_char **data, int *len);
    int initHandler(const char *type, const char *nsname, 
        const char *device, void *priv);
public:

    struct InitObject
    {
        const char *handletype;
        void *handlepriv;
        const char *device;
        const char *nsname;
        const char *filter;
        int headerOffset;

        InitObject()
        {
            memset(this, 0, sizeof(InitObject));
        }
    };

    virtual ~PacketReader();
    PacketReader(): handle(0), tsobuf(), tsolen(), filter(0) {}
    inline void setMSS(int mss)
    {
        logDebug("mss set %d", mss);
        this->mss = mss;
    }
    inline void setHeaderOffset(int off)
    {
        headerOffset = (off + 3) & (~3);
    }
    virtual int init(void *arg);
    virtual int nextIPPacket(IPPacket *dest);
    static void packetDecode(iphdr *iph, int len);
    static void m3Decode(DataMessageHeader *dh);
    inline void getLastTimestamp(timespec *tv)
    {
        *tv = lastCapture;
    }
};

}

#endif
