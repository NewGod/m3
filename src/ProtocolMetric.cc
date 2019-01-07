#include "ProtocolMetric.hh"
#include "TCPBasicMetric.hh"

namespace m3
{

#define Metric(name) ProtocolMetric::Type::name

std::map<int, int> ProtocolMetric::valueSizeMap = 
{
    { Metric(TCP_BASIC), sizeof(TCPBasicMetricValue) }
};

}
