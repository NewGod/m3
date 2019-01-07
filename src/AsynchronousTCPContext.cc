#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "AsynchronousTCPContext.hh"
#include "ThreadUtils.hh"

namespace m3
{

AsynchronousTCPContext::AsynchronousTCPContext(long interval): Context(), 
    std::thread(main, this), infomap(), lock()
{
    this->interval.tv_sec = interval / 1000000000;
    this->interval.tv_nsec = interval % 1000000000;
}

void AsynchronousTCPContext::main(AsynchronousTCPContext *me)
{
    SetRealtime();
    AssignCPU();
    while (1)
    {
        timespec *tlen = &me->interval;
        socklen_t slen = sizeof(tcp_info);

        me->lock.writeLock();
        for (auto ite = me->infomap.begin(); ite != me->infomap.end(); ++ite)
        {
            getsockopt(ite->first, SOL_TCP, TCP_INFO, &ite->second, &slen);
        }
        me->lock.writeRelease();

        nanosleep(tlen, nullptr);
    }
}

}