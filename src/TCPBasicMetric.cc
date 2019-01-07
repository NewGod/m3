#include "IPPacket.hh"
#include "KernelMessage.hh"
#include "NetworkUtils.h"
#include "PacketReader.hh"
#include "TCPOptionWalker.hh"
#include "TCPBasicMetric.hh"

namespace m3
{

TCPBasicMetric::TCPBasicMetric(): tsMap(), seqMap(), allocator(),
    sthpWindow()
{
    BlockAllocator::InitObject allocInit;

    allocInit.bs = sizeof(PacketRecord);
    allocInit.count = 100000;
    iferr (allocator.init(&allocInit) != 0)
    {
        throw 1;
    }
    iferr (sthpWindow.init(1024) != 0)
    {
        throw 2;
    }
}

void TCPBasicMetric::PacketRecord::init(IPPacketCapture *cap, unsigned ip)
{
    TCPOption *opt;
    tcphdr *tcph = (tcphdr*)((char*)cap->iph + (cap->iph->ihl << 2));
    TCPOptionWalker wk(tcph);

    left = ntohl(tcph->seq); 
    right = left + ntohs(cap->iph->tot_len) - 
        ((cap->iph->ihl << 2) + (tcph->doff << 2));
    acknum = ntohl(tcph->ack_seq);
    flags = 0;
    sack = 0;

    out = cap->iph->saddr == ip;
    ack = tcph->ack;
    time.sec = (unsigned)cap->ts.tv_sec;
    time.nsec = (unsigned)cap->ts.tv_nsec;

    while ((opt = wk.next()) != 0)
    {
        TCPOption *hd = (TCPOption*)opt;

        switch (hd->type)
        {
        case TCPOption::SACK:
            sack = opt;
            break;
        case TCPOption::TIMESTAMP:
            tsEnabled = 1;
            tsval = ntohl_noalign(opt->data);
            tsecr = ntohl_noalign(opt->data + 4);
            break;
        default:
            break;
        }
    }
}

void TCPBasicMetric::renew(void *buf)
{
    lock.writeLock();
    doRenew((IPPacketCapture*)buf);
    lock.writeRelease();
}

void TCPBasicMetric::renewType(PacketRecord *record)
{
    static const KernelMessage::Type mapping[16] = 
    {
        KernelMessage::Type::DATAIN, KernelMessage::Type::DATAIN, 
        KernelMessage::Type::DATAIN_ACK, KernelMessage::Type::DATAIN_ACK, 
        KernelMessage::Type::UNKNOWN, KernelMessage::Type::UNKNOWN, 
        KernelMessage::Type::ACK, KernelMessage::Type::ACK, 
        KernelMessage::Type::DATAOUT, KernelMessage::Type::RETX,
        KernelMessage::Type::DATAOUT, KernelMessage::Type::RETX,
        KernelMessage::Type::UNKNOWN, KernelMessage::Type::UNKNOWN,
        KernelMessage::Type::ACKOUT, KernelMessage::Type::ACKOUT,
    };
    int flag1 = record->out;
    int flag2 = record->left == record->right;
    int flag3 = record->ack;
    int flag4 = seqLess(sndnxt, record->right) && sndnxtValid;
    
    last = mapping[(flag1 << 3) | (flag2 << 2) | (flag3 << 1) | flag4];
}

void TCPBasicMetric::renewSeq(PacketRecord *record)
{
    sndnxt = 
        record->out && (!sndnxtValid || seqLess(sndnxt, record->right)) ?
        record->right : sndnxt;
    sndnxtValid = sndnxtValid || record->out;
    
    sndunaMoved = sndunaValid && seqLessEqual(snduna, record->acknum);
    snduna = (!sndunaValid && record->out) ? 
        record->left : ((!record->out && record->ack &&
        (!sndunaValid || seqLessEqual(snduna, record->acknum))) ? 
        record->acknum + 1 : snduna);
    sndunaValid = sndunaValid || (record->out || record->ack);

    rcvnxt = record->out && record->ack && 
        (!rcvnxtValid || seqLessEqual(rcvnxt, record->acknum)) ?
        record->acknum + 1 : rcvnxt;
    rcvnxtValid = rcvnxtValid || (record->out && record->ack);

    logDebug("Renewseq");
    logDebug("sndnxt %u(%d), snduna %u(%d), rcvnxt %u(%d)",
        sndnxt, sndnxtValid, snduna, sndunaValid, rcvnxt, rcvnxtValid);
}

// see RFC2018
void TCPBasicMetric::renewSACK(PacketRecord *record)
{
    firstSACK = 0;

    if (record->sack == 0)
    {
        return;
    }

    for (int i = 0; i < record->sack->len; i += 8)
    {
        unsigned left = ntohl_noalign(record->sack->data + i);
        unsigned right = ntohl_noalign(record->sack->data + (i + 4));
        
        auto ite = seqMap.upper_bound(right);
        if (ite == seqMap.end())
        {
            continue;
        }
        
        while (ite != seqMap.begin())
        {
            PacketRecord *pr = (--ite)->second;
            if (seqLessEqual(pr->right, left))
            {
                break;
            }

            // set SACK flag only if this segment is completely contained by 
            // current SACK block
            if (seqGreatEqual(pr->left, left) && seqLess(pr->right, right))
            {
                // note that we are iterating reversely, so set `first` 
                // everytime is correct
                if (!pr->sacked)
                {
                    pr->sacked = 1;
                    // only first SACK block should be used to renew sack-based 
                    // rtt sample
                    firstSACK = (i == 0) && !pr->retx ? firstSACK : &pr->time;
                }
            }
        }
    }
}

// remove & release all ACKed segments
void TCPBasicMetric::renewACK(PacketRecord *record)
{
    auto st = seqMap.begin();
    auto ed = seqMap.upper_bound(record->ackkey());
    for (auto i = st; i != ed; ++i)
    {
        ifsucc (tsMap.count(i->second->tsval) == 1)
        {
            tsMap.erase(i->second->tsval);
        }
        else
        {
            auto range = tsMap.equal_range(i->second->tsval);
            for (auto j = range.first; j != range.second; ++j)
            {
                if (j->second == i->second)
                {
                    tsMap.erase(j);
                    break;
                }
            }
        }
        allocator.release(i->second);
    }
    seqMap.erase(st, ed);
    allocator.release(record);
    pif = seqMap.size();
}

// see RFC6298(RTTM), RFC7323(timestamp option)
void TCPBasicMetric::renewRTT(PacketRecord *record)
{
    //static const unsigned long alpha = 1, beta = 2;
    unsigned long rtt = 0;

    firstACK = 0;
    
    if (!record->ack || !sndunaMoved)
    {
        return;
    }

    for (auto first = seqMap.begin();
        first != seqMap.end() && first->second->right <= record->acknum + 1;
        first++)
    {
        if (!first->second->retx)
        {
            firstACK = &first->second->time;
            break;
        }
    }

    if (firstACK)
    {
        rtt = record->time - *firstACK;
    }
    else if (firstSACK)
    {
        rtt = record->time - *firstSACK;
    }

    // if timestamp is valid
    if (rtt == 0 && record->tsEnabled && record->tsecr != 0)
    {
        // we don't have jiffies in userspace, so use our own timestamp.
        int val = tsMap.count(record->tsecr);
        ifsucc (val == 1)
        {
            auto item = tsMap.find(record->tsecr);
            rtt = record->time - item->second->time;
        }
    }

    if (rtt)
    {
        iferr (srtt == 0)
        {
            srtt = rtt << 3;
            rttvar = rtt << 2;
        }
        else
        {
            unsigned long rttdiff = rtt - (srtt >> 3);
            //rttvar = (8 - beta) * (rttvar >> 3) + 
            //    beta * ((long)rttdiff < 0 ? -rttdiff : rttdiff);
            rttvar = rttvar - (rttvar >> 2) + 
                (((long)rttdiff < 0 ? -rttdiff : rttdiff) << 1);
            //srtt = (8 - alpha) * (srtt >> 3) + alpha * rtt;
            srtt = srtt - (srtt >> 3) + rtt;
        }
    }

    logDebug("RTT %lu %lu", rttvar >> 3, srtt >> 3);
}

void TCPBasicMetric::renewSThp(PacketRecord *record)
{
    ThruputRecord trec = {record->time, record->right - record->left};

    logDebug("right %d, left %d", record->right, record->left);

    if (trec.len == 0)
    {
        return;
    }

    logDebug("THP %d %d.%u", trec.len, trec.time.sec, trec.time.nsec);

    while (!sthpWindow.isEmpty())
    {
        ThruputRecord &rec = sthpWindow.peek();

        if (record->time - rec.time > NSEC_PER_SEC)
        {
            ssum -= rec.len;
            sthpWindow.erase();
        }
        else
        {
            break;
        }
    }
    
    if (sthpWindow.isFull())
    {
        ssum -= sthpWindow.pop().len;
    }

    ssum += trec.len;
    if (sthpWindow.isEmpty())
    {
        sthp = 0;        
    }
    else
    {
        long tv = trec.time - sthpWindow.peek().time;
        tv = tv < 0.01 * NSEC_PER_SEC ? 0.01 * NSEC_PER_SEC : tv;
        sthp = ((unsigned long)ssum * NSEC_PER_SEC) / tv;
    }
    sthpWindow.push(trec);

    logDebug("THP SUM %u %lu", ssum, sthp);
}

void TCPBasicMetric::renewSet(PacketRecord *record)
{
    if (record->right - record->left == 0)
    {
        // Don't track ACK packets
        allocator.release(record);
        return;
    }

    unsigned long key = record->key();
    auto ite = seqMap.find(key);
    if (ite != seqMap.end())
    {
        // we don't renew other metadata for retransmitted packets 
        ite->second->retx = true;

        allocator.release(record);
    }
    tsMap.insert({record->tsecr, record});
    seqMap.insert({key, record});
    pif = seqMap.size();
}

void TCPBasicMetric::doRenew(IPPacketCapture *cap)
{
    PacketRecord *record;

    iferr ((record = (PacketRecord*)allocator.allocate()) == 0)
    {
        logAllocError("TCPBasicMetric::doRenew");
        return;
    }

    record->init(cap, ip);

#ifdef ENABLE_DEBUG_LOG
    logDebug("Metric %x", ip);
    PacketReader::packetDecode(cap->iph, ntohs(cap->iph->tot_len));
#endif

    renewType(record);
    renewSeq(record);
    if (record->out)
    {
        logDebug("OUT");
        renewSThp(record);
        renewSet(record);
    }
    else
    {
        renewSACK(record);
        renewRTT(record);
        renewACK(record);
    }
}

}
