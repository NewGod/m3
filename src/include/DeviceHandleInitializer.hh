#ifndef __DEVICEHANDLEINITIALIZER_HH__
#define __DEVICEHANDLEINITIALIZER_HH__

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sched.h>

#include <future>
#include <thread>

namespace m3
{

class DeviceHandle;

class DeviceHandleInitializer
{
protected:
    // credit to https://github.com/ausbin/nsdo/blob/master/nsdo.c
    int switchNS(const char *name)
    {
        char nspath[128];
        int fd;

        sprintf(nspath, "/var/run/netns/%s", name);

        iferr ((fd = open(nspath, O_RDONLY | O_CLOEXEC)) == -1)
        {
            char errbuf[64];
            logError("DeviceHandleInitializer::switchNS:" 
                "Can't open netns %s(%s)", name, strerrorV(errno, errbuf));
            return 1;
        }

        iferr (setns(fd, CLONE_NEWNET) < 0)
        {
            char errbuf[64];
            logError("DeviceHandleInitializer::switchNS:"
                "Can't switch to netns %s(%s)", name, strerrorV(errno, errbuf));
            return 2;
        }

        return 0;
    }
    virtual int init(const char *nsname, const char *device, void *priv) = 0;
    static void main(DeviceHandleInitializer *me, std::promise<int> *res, 
        const char *nsname, const char *device, void *priv)
    {
        res->set_value(me->init(nsname, device, priv));
        return;
    }
    std::thread *workthread;
    DeviceHandle *handle;
public:
    DeviceHandleInitializer() {}
    inline void run(
        std::promise<int> *res, const char *nsname, const char *device,
        void *priv)
    {
        workthread = new std::thread(main, this, res, nsname, device, priv); 
    }
    inline DeviceHandle *getHandle()
    {
        return handle;
    }
    inline void join()
    {
        workthread->join();
    }
};

}

#endif
