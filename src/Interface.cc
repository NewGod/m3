#include <cstring>

#include <pcap/pcap.h>

#include "DataMessage.hh"
#include "Interface.hh"
#include "Log.h"
#include "Reflect.hh"

namespace m3
{

AsynchronousTCPContext* Interface::aTCPCxt = nullptr;

Interface::~Interface()
{
    if (fm)
    {
        delete fm;
    }
#ifdef ENABLE_CWND_AWARE
    if(fd >= 0)
        aTCPCxt->remove(fd);
#endif
}

Interface::Interface() : name(""), ip(0), mask(0), fd(-1), fs(0), mp(0), fm(0),
    pending(0)
{
#ifdef ENABLE_CWND_AWARE
    if(!aTCPCxt)
        aTCPCxt = snew AsynchronousTCPContext(0);
#endif
}

int Interface::setMTU()
{
    char buf[128];
    FILE *msgfile;
    int ret;
    sprintf(buf, "cat /sys/class/net/%s/mtu; echo 0", name);

    iferr ((msgfile = popen(buf, "r")) == 0)
    {
        char errbuf[64];
        logError("Interface::setMTU: popen failed(%s)",
            strerrorV(errno, errbuf));
        return 1;
    }

    iferr (fgets(buf, 16, msgfile) == 0)
    {
        char errbuf[64];
        logError("Interface::setMTU: pipe read failed(%s)",
            strerrorV(errno, errbuf));
        return 2;
    }

    iferr ((ret = pclose(msgfile)) < 0)
    {
        char errbuf[64];
        logError("Interface::setMTU: Can't get return value(%s)",
            strerrorV(errno, errbuf));
        return 3;
    }

    iferr ((mtu = atoi(buf)) == 0)
    {
        logError("Interface::setMTU: Zero MTU.");
        return 4;
    }

    return 0;
}

int Interface::init(void *arg)
{
    InitObject *iarg = (InitObject*)arg;
    char errbuf[PCAP_ERRBUF_SIZE];
    socklen_t socklen = sizeof(mss);
    int ret;

    strncpy((char*)name, iarg->device, NAME_LEN);

    iferr (pcap_lookupnet(iarg->device, 
        (unsigned*)&ip, (unsigned*)&mask, errbuf) != 0)
    {
        logError("Interface::init: pcap_lookupnet failed(%s)",
            errbuf);
        return 1;
    }

    iferr (getsockopt(iarg->initFM.fd, IPPROTO_TCP, 2, &mss, &socklen) < 0)
    {
        char errbuf[64];
        logError("Interface::init: Cannot get MSS.(%s)", 
            strerrorV(errno, errbuf));
        return 2;
    }
    
    fd = iarg->initFM.fd;
    if(aTCPCxt)
        aTCPCxt->watch(fd);

    iferr ((ret = setMTU()) != 0)
    {
        logStackTrace("Interface::init", ret);
        return 3;
    }

    iferr ((fm = snew ForwardModule()) == 0)
    {
        logAllocError("Interface::init");
        return 5;
    }

    iarg->initFM.master = this;
    iferr ((ret = fm->init(&iarg->initFM)) != 0)
    {
        logStackTrace("Interface::init", ret);
        return 6;
    }
    
    iferr ((fs = (FileStream*)Reflect::create(iarg->fsname)) == 0)
    {
        logStackTrace("Interface::init", ret);
        return 7;
    }

    fs->bind(iarg->initFM.fd);

    this->mp = iarg->provider;

    return 0;
}

bool Interface::isReady()
{
    return pending.load() == 0;
}

int Interface::send(DataMessage *msg)
{
    pending = msg;
    int ret = msg->write(fs);
    //char src[64];
    //logDebug("SEND IP %s", inet_ntoa_r(*(in_addr*)&ip, src));
    iferr (ret < 0)
    {
        logStackTrace("Interface::send", ret);
    }
    else if (msg->written())
    {
        pending = 0;
    }
    return ret;
}

}