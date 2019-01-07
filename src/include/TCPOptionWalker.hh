#ifndef __TCPOPTIONWALKER_HH__
#define __TCPOPTIONWALKER_HH__

#include <netinet/tcp.h>

namespace m3
{

// Note: a _cleverer_ way of implementing this is remove the `data` field below
// and define a class for each option type with their own data structure.
// BUT this is buggy because the multi-byte fields are not necessarily aligned.
// use ntoh*_noalign() to acquire field value instead.
struct TCPOption
{
    enum Type 
    {
        END_OF_LIST = 0,
        NO_OPERATION = 1,
        MAX_SEG_SIZE = 2,
        SACK = 5,
        TIMESTAMP = 8
    };

    unsigned char type;
    unsigned char len;
    unsigned char data[];
};

class TCPOptionWalker
{
protected:
    tcphdr *tcph;
    char* current;
    char* ceiling;
public:
    static const int TCP_OPTION_OFFSET = 20;
    
    void init(tcphdr *tcphd)
    {
        tcph = tcphd;
        current = 0;
        ceiling = (char*)tcphd + ((tcphd->doff) << 2);
    }
    
    TCPOptionWalker(tcphdr *tcphd)
    {
        init(tcphd);
    }

    TCPOptionWalker() {}

    inline TCPOption* next()
    {
        if (current == 0)
        {
            current = (char*)tcph + TCP_OPTION_OFFSET;
        }
        // According to RFC793, EOL and NOP are the only options without 
        // `length` field.
        else if (*current == TCPOPT_EOL || *current == TCPOPT_NOP)
        {
            current++;
        }
        else
        {
            current += *(char*)(current + 1);
        }

        return (TCPOption*)(current >= ceiling ? 0 : current);
    }
};

}
#endif
