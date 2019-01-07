#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "Represent.h"
#include "RobustFileStream.hh"

namespace m3
{

RobustFileStream::RobustFileStream()
{
    rio = alloc(rio_t);
}

RobustFileStream::~RobustFileStream()
{
    free(rio);
}

ssize_t RobustFileStream::read(void *buf, size_t count)
{
    ssize_t ret = rio_readnb(rio, buf, count);
    if (ret == -1)
    {
        this->err = errno;
        logError("RobustFileStream::read: rio_readnb failed(%s).", 
            strerrorV(err, errbuf));
    }
    return ret;
}

ssize_t RobustFileStream::write(void *buf, size_t count)
{
    ssize_t ret = rio_writen(fd, buf, count);
    if (ret == -1)
    {
        this->err = errno;
        logError("RobustFileStream::write: rio_writen failed(%s).", 
            strerrorV(err, errbuf));
    }

    logDebug("RobustFileStream::write: Written %lu bytes", count);

    return ret;
}

int RobustFileStream::bind(int fd)
{
    int enable = 1;
    FileStream::bind(fd);
    rio_readinitb(rio, fd);
    if (setsockopt(fd, IPPROTO_TCP, 
        TCP_NODELAY, (void*)&enable, sizeof(enable)) != 0)
    {
        char errbuf[64];
        logError("RobustFileStream::bind: setsockopt() for %d failed(%s)",
            fd, strerrorV(errno, errbuf));
        return 1;
    }
    return 0;
}

}