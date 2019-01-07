#ifndef __ROBUSTFILESTREAM_HH__
#define __ROBUSTFILESTREAM_HH__

#include "FileStream.hh"
#include "NetworkUtils.h"

namespace m3
{

class RobustFileStream : public FileStream
{
public:
    ssize_t read(void *buf, size_t count) override;
    ssize_t write(void *buf, size_t count) override;
    int bind(int fd) override;
    RobustFileStream();
    virtual ~RobustFileStream();
    static void* newInstance() //@ReflectRobustFileStream
    {
        return snew RobustFileStream();
    }
protected:
    rio_t *rio;
};

}

#endif
