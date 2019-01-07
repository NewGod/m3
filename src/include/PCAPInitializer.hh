#ifndef __PCAPINITIALIZER_HH__
#define __PCAPINITIALIZER_HH__

#include "DeviceHandleInitializer.hh"
#include "LivePCAP.hh"
#include "Represent.h"

namespace m3
{

class PCAPInitializer : public DeviceHandleInitializer
{
public:
    static void* newInstance() //@ReflectPCAPInitializer
    {
        return snew PCAPInitializer();
    }
protected:
    int init(const char *nsname, const char *device, void *priv) override
    {
        LivePCAP::InitObject iobj;
        int ret;

        iferr (nsname != 0 && (ret = switchNS(nsname)) != 0)
        {
            logStackTrace("PCAPInitializer::init", ret);
            return 1;
        }
        
        iferr ((handle = snew LivePCAP()) == 0)
        {
            logAllocError("PCAPInitializer::init");
            return 2;
        }
        
        iobj.device = device;
        iobj.snaplen = *((int*)priv);
        iferr ((ret = ((LivePCAP*)handle)->init(&iobj)) != 0)
        {
            logStackTrace("PCAPInitializer::init", ret);
            return 3;
        }

        return 0;
    }
};

}


#endif
