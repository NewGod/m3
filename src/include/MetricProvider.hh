#ifndef __METRICPROVIDER_HH__
#define __METRICPROVIDER_HH__

#include "ProtocolMetric.hh"

namespace m3
{

class MetricProvider
{
public:
    virtual ~MetricProvider() {};

    virtual void renewMetric(void *buf) = 0;
    virtual int getMetric(ProtocolMetric::Type id, 
        ProtocolMetric **dest) = 0;
    virtual int addMetric(ProtocolMetric* metric) = 0;
};

}

#endif
