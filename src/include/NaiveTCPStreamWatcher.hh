#ifndef __NAIVETCPSTREAMWATCHER_HH__
#define __NAIVETCPSTREAMWATCHER_HH__

#include "ArrayMetricProvider.hh"
#include "PacketReader.hh"
#include "Represent.h"
#include "RobustFileStream.hh"

namespace m3
{

class NaiveTCPMessageProvider;

// This is a _really_ naive implement, this implement try to use an 
// asynchronous thread to sniff packets and renew TCP metric. As for
// message delivery, we read TCP data by using read().
class NaiveTCPStreamWatcher : public PacketReader, public ArrayMetricProvider
{
protected:
    class MetricWorker : public std::thread
    {
    protected:
        static void tmain(NaiveTCPStreamWatcher *ntcpw);
    public:
        MetricWorker(NaiveTCPStreamWatcher *ntcpw): 
            std::thread(tmain, ntcpw) {};
    };

    static const int MAX_METRIC = 8;
    static const int TCPMETRIC_OFF = 0x00060000;
    virtual int getIndex(int ind) override
    {
        return (ind >= TCPMETRIC_OFF && ind < TCPMETRIC_OFF + MAX_METRIC) ?
            ind - TCPMETRIC_OFF : -1;
    }

    MetricWorker *mw;
    RobustFileStream is;
    int fd;
public:
    struct InitObject
    {
        PacketReader::InitObject prInit;
        sockaddr_in myaddr;
        sockaddr_in peeraddr;
        ProtocolMetric **metrics;
        int metricCount;
        int fd;

        InitObject()
        {
            memset(this, 0, sizeof(InitObject));
        }
    };

    NaiveTCPStreamWatcher(): PacketReader(), ArrayMetricProvider(), mw(0),
        is(), fd(0) {}
    virtual ~NaiveTCPStreamWatcher();

    virtual int init(void *arg) override;
    virtual int nextIPPacket(IPPacket *dest) override;

    static void* newInstance() //@ReflectNaiveTCPStreamWatcher
    {
        return snew NaiveTCPStreamWatcher();
    }

    friend class NaiveTCPMessageProvider;
};

}

#endif

