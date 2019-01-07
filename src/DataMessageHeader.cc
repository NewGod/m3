#include <arpa/inet.h>

#include "DataMessageHeader.hh"
#include "Log.h"
#include "NetworkUtils.h"
#include "TCPOptionWalker.hh"

namespace m3
{

const int DataMessageHeader::TCPH_OFF = offsetOf(tcph, m3::DataMessageHeader);
const int DataMessageHeader::LEN_OFF = offsetOf(saddr, m3::DataMessageHeader);
const int DataMessageHeader::MAX_TCPHDR_LEN = 60;
const int DataMessageHeader::MSS_REDUCE_VAL = 
    DataMessageHeader::TCPH_OFF + DataMessageHeader::MAX_TCPHDR_LEN;

int DataMessageHeader::modifyMSS(int tgt)
{
    TCPOptionWalker wk(&tcph);
    TCPOption *opt;
    bool modified = false;

    while ((opt = wk.next()))
    {
        if (opt->type == (unsigned char)TCPOption::Type::MAX_SEG_SIZE)
        {
            unsigned short mss = ntohs_noalign(opt->data);
            unsigned short newmss = mss < tgt ? mss : tgt;

            htons_noalign(opt->data, newmss);
            modified = true;

            logDebug("DataMessageHeader::modifyMSS: MSS %d -> %d",
                mss, newmss);
        }
    }

    iferr (!modified)
    {
        logWarning("DataMesssageHeader::modifyMSS: No MSS option found!");
    }

    return !modified;
}

}