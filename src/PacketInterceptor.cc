#include <arpa/inet.h>
#include <linux/if_tun.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <errno.h>
#include <cstdio>
#include <cstdlib>

#include "Connection.hh"
#include "LivePCAP.hh"
#include "Log.h"
#include "NetworkUtils.h"
#include "PacketInterceptor.hh"
#include "Reflect.hh"
#include "Represent.h"

namespace m3
{

PacketInterceptor::PacketInterceptor(): peer(), exec(false) {}

PacketInterceptor::~PacketInterceptor()
{
    if (exec)
    {
        char revscript[512];
        sprintf(revscript, "%s -c", script);
        if (system(revscript) != 0)
        {
            logError("PacketInterceptor::PacketInterceptor: "
                "Failed to undo network setting(command: %s)", revscript);
        }
    }
}

const char *PacketInterceptor::getNSName()
{
    return nsname;
}

int PacketInterceptor::makeDevice(const char* srcdev, const char *fwddev,
    unsigned fwdip, const char *peerdev, unsigned peerip, const char *args,
    int fwmark)
{
    char msg[128];
    int retcode;
    char fwdipstr[16];
    char peeripstr[16];
    FILE *msgfile;
    
    strcpy(fwdipstr, inet_ntoa(*(in_addr*)&fwdip));
    strcpy(peeripstr, inet_ntoa(*(in_addr*)&peerip));

    if (args)
    {
        sprintf(script, "./m3-initdevice.sh"
            " -D %s -A %s -d %s -a %s -t %s -n m3ns%d %s -M %d", fwddev,
                fwdipstr, peerdev, peeripstr, srcdev, getpid(), args, fwmark);
    }
    else
    {
        sprintf(script, 
            "./m3-initdevice.sh -D %s -A %s -d %s -a %s -t %s -n m3ns%d -M %d",
            fwddev, fwdipstr, peerdev, peeripstr, srcdev, getpid(), fwmark);
    }

    logDebug("Command: %s", script);

    iferr ((msgfile = popen(script, "r")) == 0)
    {
        char errbuf[64];
        logError("PacketInterceptor::makeDevice: popen failed(%s)",
            strerrorV(errno, errbuf));
        return 1;
    }

    iferr (fgets(msg, 128, msgfile) == 0)
    {
        char errbuf[64];
        logError("PacketInterceptor::makeDevice: pipe read failed(%s)",
            strerrorV(errno, errbuf));
        return 2;
    }

    iferr ((retcode = pclose(msgfile)) < 0)
    {
        char errbuf[64];
        logError("PacketInterceptor::makeDevice: Can't get return value(%s)",
            strerrorV(errno, errbuf));
        return 3;
    }
    else iferr (retcode != 0)
    {
        msg[strlen(msg) - 1] = 0;
        logError("PacketInterceptor::makeDevice: Device create failed(%d: %s)",
            retcode, msg);
        return 4;
    }

    exec = 1;
    return 0;
}

int PacketInterceptor::init(void *arg)
{   
    int ret;
    InitObject *iarg = (InitObject*)arg;
    PacketReader::InitObject prInit;
    char peerdev[IFNAMSIZ + 1];
    unsigned peerip = htonl(ntohl(iarg->fwdip) ^ 1);

    iferr (strlen(iarg->fwddev) + 5 > IFNAMSIZ)
    {
        logError("PacketInterceptor::init: device name %s too long(max %d).",
            iarg->fwddev, IFNAMSIZ - 5);
        return 1;
    }
    sprintf(peerdev, "%s-peer", iarg->fwddev);
    iferr ((ret = makeDevice(iarg->srcdev, iarg->fwddev, iarg->fwdip, 
        peerdev, peerip, iarg->scriptArgs, iarg->fwmark)) != 0)
    {
        logStackTrace("PacketInterceptor::init", ret);
        return 2;
    }

    sprintf(nsname, "m3ns%d", getpid());
    prInit.handletype = iarg->handletype;
    prInit.handlepriv = iarg->handlepriv;
    prInit.device = iarg->fwddev;
    prInit.nsname = nsname;
    prInit.filter = iarg->filter;
    prInit.headerOffset = sizeof(ethhdr);
    iferr ((ret = PacketReader::init(&prInit)) != 0)
    {
        logStackTrace("PacketInterceptor::init", ret);
        return 3;

    }
    
    iferr ((ret = this->peer.setMetadata(peerdev)) != 0)
    {
        logStackTrace("PacketInterceptor::init", ret);
        return 4;
    }

    return 0;
}

int PacketInterceptor::forward(DataMessage *msg)
{
    IPPacket *pak;
    int ret;
    DeviceMetadata dm;

    pak = msg->toIPPacket();

    DataMessageHeader *dh = (DataMessageHeader*)pak->dataHeader;

#ifdef ENABLE_DEBUG_LOG
    int tcplen = dh->getTCPLen();
    logDebug("PacketInterceptor::forward:");
    PacketReader::m3Decode((DataMessageHeader*)pak->dataHeader);
#endif 

    memcpy(dm.mac, this->handle->me.mac, ETH_ALEN);
    dm.ip = dh->saddr;
    ethhdr *ethh = pak->toRaw(&dm, &this->peer);

#ifdef ENABLE_DEBUG_LOG
    if (tcplen != 0)
    {
        PacketReader::packetDecode(pak->getIPHeader(), pak->getIPLen());
    }
#endif

    iferr ((ret = this->handle->sendPacket(ethh, 
        pak->len - ((char*)ethh - pak->header))) != 0)
    {
        logStackTrace("PacketInterceptor::forward", ret);
        return 1;
    }

    return 0;
}

int PacketInterceptor::next(IPPacket *dest)
{
    int ret = PacketReader::nextIPPacket(dest);
    ifsucc (ret == 0)
    {
        logDebug("iph %lx, len %d", 
            (unsigned long)dest->getIPHeader(), dest->getIPLen());
#ifdef ENABLE_DEBUG_LOG
        packetDecode(dest->getIPHeader(), dest->getIPLen());
#endif
        dest->initRaw();
    }
    else
    {
        logStackTrace("PacketInterceptor::next", ret);
    }
    return ret != 0;
}

}
