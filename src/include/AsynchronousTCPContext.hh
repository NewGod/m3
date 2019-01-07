#ifndef __ASYNCHRONOUSTCPCONTEXT_HH__
#define __ASYNCHRONOUSTCPCONTEXT_HH__

#include <netinet/tcp.h>
#include <map>
#include <thread>

#include "RWLock.hh"
#include "Context.hh"

namespace m3
{

class AsynchronousTCPContext : public Context,
    public std::thread
{
private:
    static void main(AsynchronousTCPContext *me);
    std::map<int, tcp_info> infomap;
    RWLock lock;
    timespec interval;
public:
    AsynchronousTCPContext(long interval = 0);
    Type getType() override
    {
        return Type::ASYNTCP;
    }

    void watch(int fd)
    {
        lock.writeLock();
        infomap.insert({ fd, tcp_info() });
        lock.writeRelease();
    }
    void remove(int fd)
    {
        lock.writeLock();
        infomap.erase(fd);
        lock.writeRelease();
    }
    int getInfo(int fd, tcp_info *dest)
    {
        lock.readLock();
        auto ite = infomap.find(fd);
        if (ite == infomap.end())
        {
            lock.readRelease();
            return 1;
        }
        
        *dest = ite->second;
        lock.readRelease();
        return 0;
    }
};

}

#endif
