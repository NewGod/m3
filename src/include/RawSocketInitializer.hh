#ifndef __RAWSOCKETINITIALIZER_HH__
#define __RAWSOCKETINITIALIZER_HH__

#include "DeviceHandleInitializer.hh"
#include "RawSocket.hh"
#include "Represent.h"

namespace m3
{

class RawSocketInitializer : public DeviceHandleInitializer
{
public:
    static void* newInstance() //@ReflectRawSocketInitializer
    {
        return snew RawSocketInitializer();
    }
protected:
    int init(const char *nsname, const char *device, void *priv) override
    {
        int ret;

        iferr (nsname != 0 && (ret = switchNS(nsname)) != 0)
        {
            logStackTrace("RawSocketInitializer::init", ret);
            return 1;
        }
        
        iferr ((handle = snew RawSocket()) == 0)
        {
            logAllocError("RawSocketInitializer::init");
            return 2;
        }

        iferr((ret = ((RawSocket*)handle)->init(device)) != 0)
        {
            logStackTrace("RawSocketInitializer::init", ret);
            return 3;
        }

        return 0;
    }
};

}

#endif
