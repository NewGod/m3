#ifndef __PACKETINTERCEPTOR_HH__
#define __PACKETINTERCEPTOR_HH__

#include "DeviceMetadata.hh"
#include "LivePCAP.hh"
#include "PacketSource.hh"
#include "PacketReader.hh"

namespace m3
{

class InterfaceManager;

class PacketInterceptor : public PacketSource, public PacketReader
{
public:
    PacketInterceptor();
    virtual ~PacketInterceptor();
    struct InitObject
    {
        const char *srcdev;
        const char *fwddev;
        unsigned srcip;
        unsigned fwdip;
        const char *filter;
        const char *handletype;
        void *handlepriv;
        const char *scriptArgs;
        int fwmark;
    };
    static void* newInstance() //@ReflectPacketInterceptor
    {
        return snew PacketInterceptor();
    }
    int init(void *arg) override;
    virtual int forward(DataMessage *msg) override;
    int next(IPPacket *dest) override;
    int makeDevice(const char* srcdev, const char *fwddev, unsigned fwdip, 
        const char *peerdev, unsigned peerip, const char *args, int fwmark);
    const char *getNSName();
    static int switchNS(const char *name);
protected:
    static const int MIN_PAK_HEADER_LEN = 
        sizeof(iphdr) + sizeof(ethhdr);

    InterfaceManager *intfmgr;
    DeviceMetadata peer;
    char script[256];
    bool exec;
    char nsname[32];
};

}

#endif
