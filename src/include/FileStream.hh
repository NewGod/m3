#ifndef __FILESTREAM_HH__
#define __FILESTREAM_HH__

#include <errno.h>
#include <unistd.h>

#include "Log.h"

namespace m3
{

static int (*closeFD)(int) = close;
static ssize_t (*readFD)(int, void*, size_t) = read;
static ssize_t (*writeFD)(int, const void*, size_t) = write;

class FileStream
{
protected:
    int fd;
    int err;
    char errbuf[32];
    bool cleanOnDestroy;
public:
    virtual ~FileStream()
    {
        if (cleanOnDestroy)
        {
            closeFD(fd);
        }
    }
    inline int getErrorNum()
    {
        return err;
    }
    inline int getFD()
    {
        return fd;
    }
    inline void setClean(bool val)
    {
        cleanOnDestroy = val;
    }
    inline bool getClean()
    {
        return cleanOnDestroy;
    }
    virtual ssize_t read(void *buf, size_t count)
    {
        int ret = readFD(fd, buf, count);
        this->err = errno;
        if (ret == -1)
        {
            logError("FileStream::read: read failed(%s).", 
                strerrorV(err, errbuf));
        }
        return ret;
    }
    virtual ssize_t write(void *buf, size_t count)
    {
        int ret = writeFD(fd, buf, count);
        this->err = errno;
        if (ret == -1)
        {
            logError("FileStream::write: write failed(%s).", 
                strerrorV(err, errbuf));
        }
        return ret;
    }
    virtual int bind(int fd)
    {
        this->fd = fd;
        return 0;
    }
    virtual int close()
    {
        int ret = closeFD(fd);
        this->err = errno;
        if (ret == -1)
        {
            logError("FileStream::close: close failed(%s).", 
                strerrorV(err, errbuf));
        }
        return ret;
    }
    FileStream() {}
    FileStream(int fd)
    {
        bind(fd);
    }
    static void* newInstance() //@ReflectFileStream
    {
        return snew FileStream();
    }
};

}

#endif
