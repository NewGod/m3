#ifndef __ARRAYMETRICPROVIDER_HH__
#define __ARRAYMETRICPROVIDER_HH__

#include <string.h> 

#include "Log.h"
#include "MetricProvider.hh"
#include "Represent.h"

namespace m3
{

class ArrayMetricProvider : public MetricProvider
{
protected:
    virtual int getIndex(int id) = 0;
    ProtocolMetric **metrics;
    int metricCount;
public:
    ArrayMetricProvider(): metrics(0), metricCount(0) {}
    ArrayMetricProvider(int cnt)
    {
        init(cnt);
    }

    int init(int cnt)
    {
        iferr ((metrics = snew ProtocolMetric*[cnt]) == 0)
        {
            logAllocError("ArrayMetricProvider::init");
            return 1;
        }
        memset(metrics, 0, sizeof(ProtocolMetric*) * cnt);
        metricCount = cnt;
        return 0;
    }
    int addMetric(ProtocolMetric* metric) override;
    void renewMetric(void *buf) override;
    int getMetric(
        ProtocolMetric::Type id, ProtocolMetric **dest) override;
};

}

#endif

