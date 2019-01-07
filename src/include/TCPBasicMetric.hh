#ifndef __TCPBASICMETRIC_HH__
#define __TCPBASICMETRIC_HH__

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/time.h>

#include <cstring>
#include <ctime>
#include <map>
#include <unordered_map>

#include "BlockAllocator.hh"
#include "DataMessageHeader.hh"
#include "FixQueue.hh"
#include "IPPacketCapture.hh"
#include "KernelMessage.hh"
#include "PacketReader.hh"
#include "ProtocolMetric.hh"
#include "RWLock.hh"
#include "TCPOptionWalker.hh"

namespace m3
{

struct TCPBasicMetricValue
{
    static const unsigned NSEC_PER_SEC = 1000000000;
    struct AliasTimespec
    {
        int sec;
        unsigned nsec;

        inline operator long()
        {
            return (long)sec * NSEC_PER_SEC + nsec;
        }
        inline AliasTimespec operator-(const AliasTimespec &rv) const
        {
            return { sec - rv.sec - (nsec < rv.nsec), nsec - rv.nsec +
                (nsec < rv.nsec ? NSEC_PER_SEC : 0) };
        }
        inline bool operator<(const AliasTimespec &rv) const
        {
            return long(*this - rv) < 0;
        }
    };

    // TCP sequence number related value, as specified in RFC793
    unsigned snduna;
    unsigned sndnxt;
    unsigned rcvnxt;

    // Flow metrics.
    // timeunit: 1/8 nanosecond
    // recommended usage: right shift 13 bits to transform to a unit quite 
    // similar to microsecond(1024 nanosecond)
    unsigned long srtt;
    unsigned long rttvar;
    unsigned long sthp; //unit: B
    unsigned ssum;
    unsigned pif; //num of packets in flight
    //unsigned loss;

    //long queueing;

    // Per-packet metric
    KernelMessage::Type last;
protected:
    AliasTimespec *firstSACK;
    AliasTimespec *firstACK;
public:
    // flags
    union
    {
        struct
        {
            unsigned sndunaValid : 1;
            unsigned sndnxtValid : 1;
            unsigned rcvnxtValid : 1;
            unsigned sndunaMoved : 1;
        };
        unsigned flags;
    };

    TCPBasicMetricValue(): srtt(0), ssum(0), pif(0),
        sndunaValid(0), sndnxtValid(0), rcvnxtValid(0) {}
};

// We assume that we started measure work right after the connection has
// established.
class TCPBasicMetric : public ProtocolMetric, protected TCPBasicMetricValue
{
protected:
    typedef BlockAllocator PacketRecordAllocator;
    class PacketRecord
    {
    public:
        struct Hash
        {
            unsigned operator()(const PacketRecord* const& rec) const
            {
                return rec->tsval;
            }
        };
        struct KeyLess
        {
            bool operator()(
                const unsigned long &a, const unsigned long &b) const
            {
                unsigned ar = a >> 32;
                unsigned al = a;
                unsigned br = b >> 32;
                unsigned bl = b;
                return seqLess(ar, br) ? 1 : seqLess(al, bl);
            }
        };

        // timestamp
        AliasTimespec time;
        // seq numbers
        unsigned left;
        unsigned right;
        unsigned acknum;
        // TCP timestamp option value
        unsigned tsval;
        unsigned tsecr;
        // SACK Block pointer, valid only when processing ACK
        TCPOption* sack;
        union
        {
            int flags;
            struct
            {
                int out : 1;
                int ack : 1;
                int retx : 1;
                int sacked : 1;
                int tsEnabled : 1;
            };
        };

        inline unsigned long key() const
        {
            return ((unsigned long)this->right << 32) | this->left;
        }
        inline unsigned long ackkey() const
        {
            unsigned long ackseq = (unsigned long)this->acknum + 1;
            return (ackseq << 32) | ackseq;
        }
        bool operator==(const PacketRecord& rv) const
        {
            return this->time.sec == rv.time.sec &&
                this->time.nsec == rv.time.nsec;
        }
        PacketRecord() {}
        void init(IPPacketCapture *iph, unsigned ip);
    };
    struct ThruputRecord
    {
        AliasTimespec time;
        unsigned len;
    };

    void renewType(PacketRecord *record);
    void renewSeq(PacketRecord *record);
    void renewRTT(PacketRecord *record);
    void renewSet(PacketRecord *record);
    void renewSACK(PacketRecord *record);
    void renewACK(PacketRecord *record);
    void renewSThp(PacketRecord *record);
    void doRenew(IPPacketCapture *iph);

    unsigned ip;
    std::unordered_multimap<
        unsigned,
        PacketRecord*> tsMap;
    std::map<
        unsigned long,
        PacketRecord*,
        PacketRecord::KeyLess> seqMap;
    PacketRecordAllocator allocator;
    FixQueue<ThruputRecord> sthpWindow;
public:
    TCPBasicMetric();
    ProtocolMetric::Type getID() override
    {
        return ProtocolMetric::Type::TCP_BASIC;
    }
    void renew(void *buf) override;
    const void* getValue() override
    {
        return (TCPBasicMetricValue*)this;
    }
    inline void setIP(unsigned ip)
    {
        this->ip = ip;
    }
    static void* newInstance() //@ReflectTCPBasicMetric
    {
        return snew TCPBasicMetric();
    }
    static inline bool seqLess(unsigned seq1, unsigned seq2)
    {
        return (int)(seq1 - seq2) < 0;
    }
    static inline bool seqGreat(unsigned seq1, unsigned seq2)
    {
        return seqLess(seq2, seq1);
    }
    static inline bool seqLessEqual(unsigned seq1, unsigned seq2)
    {
        return (int)(seq1 - seq2) <= 0;
    }
    static inline bool seqGreatEqual(unsigned seq1, unsigned seq2)
    {
        return seqLessEqual(seq2, seq1);
    }
    static inline int gettime(timespec *res)
    {
        return clock_gettime(CLOCK_MONOTONIC_COARSE, res);
    }
};

}

#endif
