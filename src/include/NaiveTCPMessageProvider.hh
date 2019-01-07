#ifndef __NAIVETCPMESSAGEPROVIDER_HH__
#define __NAIVETCPMESSAGEPROVIDER_HH__

#include "KernelMessageProvider.hh"
#include "NaiveTCPStreamWatcher.hh"

namespace m3
{

class KernelMessage;
class DeviceHandle;

class NaiveTCPMessageProvider : public KernelMessageProvider
{
public:
    struct InitObject
    {
        sockaddr_in myaddr;
        sockaddr_in peeraddr;
        const char *handletype;
        void *handlepriv;
        const char *device;
        int fd;

        InitObject()
        {
            memset(this, 0, sizeof(InitObject));
        }
    };

    NaiveTCPMessageProvider() {}
    virtual int next(KernelMessage **dest) override;
    int init(void *arg) override;
    static void* newInstance() //@ReflectNaiveTCPMessageProvider
    {
        return snew NaiveTCPMessageProvider();
    }
};

}

#endif
