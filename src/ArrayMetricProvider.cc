#include "ArrayMetricProvider.hh"
#include "Log.h"
#include "Represent.h"

namespace m3
{

int ArrayMetricProvider::addMetric(ProtocolMetric* metric)
{
    int id = getIndex(metric->getID());
    iferr (id == -1)
    {
        logError("ArrayMetricProvider::addMetric: Invalid ID %d.", 
            metric->getID());
    }
    else if (metrics[id] != 0)
    {
        logError("ArrayMetricProvider::addMetric: Metric %d already exists.",
            metric->getID());
    }
    else
    {
        metrics[id] = metric;
        return 0;
    }
    return 1;
}

void ArrayMetricProvider::renewMetric(void *buf)
{
    for (int i = 0; i < metricCount; ++i)
    {
        if (metrics[i] != 0)
        {
            metrics[i]->renew(buf);
        }
    }
}

int ArrayMetricProvider::getMetric(
    ProtocolMetric::Type id, ProtocolMetric **dest)
{
    int ind = getIndex(id);
    iferr (ind == -1)
    {
        logError("ArrayMetricProvider::getMetric: Invalid ID %d.", id);
    }
    else if (metrics[ind] != 0)
    {
        *dest = metrics[ind];
        return 0;
    }
    else
    {
        logError("ArrayMetricProvider::getMetric: Metric %d not included", id);
    }
    return 1;
}

}