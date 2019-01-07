#ifndef __PROTOCOLMETRIC_HH__
#define __PROTOCOLMETRIC_HH__

#include <netinet/ip.h>

#include <map>

#include "ConcurrentValueHolder.hh"
#include "Log.h"
#include "RWLock.hh"

namespace m3
{

// How to add a new Type:
// - add an item in Type
// - add an item in valueSizeMap's initialzation list(see ProtocolMetric.cc)
// Type value constraints for your ProtocolMetric:
// - Format of MetricValue:
//    struct Type
//    {
//        union
//        {
//            struct
//            {
//                unsigned char pref;
//                unsigned char protocol;
//                unsigned short ind;
//            };
//            int value;
//        };
//    };
// - For a layer-4 protocol, pref = 0, protocol = the value used in `protocol`
//   field of layer-3 headers.
// - For other protocols(probably defined by yourself!), pref != 0
class ProtocolMetric : public ConcurrentValueHolder
{
protected:
    static std::map<int, int> valueSizeMap;
    RWLock lock;
public:
    enum Type { CONN_META = 0xFFFFFFFF, TCP_BASIC = 0x00060000 };
    static inline int getMetricSize(Type id)
    {
        auto ite = valueSizeMap.find(id);
        if (ite != valueSizeMap.end())
        {
            return ite->second;
        }
        logWarning("ProtocolMetric::getMetricSize: Unknown metric type %d.", 
            id);
        return -1;
    }

    ProtocolMetric(): lock() {}

    virtual Type getID() = 0;
    virtual void renew(void *buf) = 0;
};

}

#endif
