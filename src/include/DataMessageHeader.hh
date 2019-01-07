#ifndef __DATAMESSAGEHEADER_HH__
#define __DATAMESSAGEHEADER_HH__

#include <netinet/tcp.h>

#include <cstdint>

#include "Log.h"
#include "Represent.h"

namespace m3
{

struct DataMessageMetaHeader
{
    enum Type { NONE, USERDATA, INTERNAL, count };
    enum UserDataSubtype { UD_NONE, UD_DATA, UD_count };
    enum InternalSubtype { I_GENERIC, I_count };

    static const uint16_t magic1 = 0xDEAD;
    static const uint16_t magic2 = 0xBEEF;

    std::uint8_t type;
    std::uint8_t subtype;
    std::uint16_t tlen;
    std::uint16_t lenchk1;
    std::uint16_t lenchk2;

    inline void generateChk()
    {
        lenchk1 = tlen - magic1;
        lenchk2 = tlen + magic2;
    }
    inline bool strongCheck()
    {
        uint16_t len1 = lenchk1 + magic1;
        uint16_t len2 = lenchk2 - magic2;

        return tlen == len1 && tlen == len2 && len1 == len2;
    }
    inline bool weakCheck()
    {
        uint16_t len1 = lenchk1 + magic1;
        uint16_t len2 = lenchk2 - magic2;
 
        if (tlen == len1 || tlen == len2)
        {
            return true;
        }
        else if (len1 == len2)
        {
            tlen = len1;
            return true;
        }
        return false;
    }
};

// mostly inherited from <netinet/tcp.h>
struct DataMessageHeader : public DataMessageMetaHeader
{
    std::uint32_t saddr;
    std::uint32_t daddr;
    static const int TCPH_OFF;
    static const int LEN_OFF;
    static const int MAX_TCPHDR_LEN;
    static const int MSS_REDUCE_VAL;
    union
    {
        tcphdr tcph;
        struct
        {
            std::uint16_t sport;
            std::uint16_t dport;
            std::uint32_t seq;
            std::uint32_t ackseq;
            std::uint16_t tcpres1 : 4;
            std::uint16_t doff : 4;
            std::uint16_t fin : 1;
            std::uint16_t syn : 1;
            std::uint16_t rst : 1;
            std::uint16_t psh : 1;
            std::uint16_t ack : 1;
            std::uint16_t urg : 1;
            std::uint16_t tcpres2 : 2;
            std::uint16_t window;
            std::uint16_t check;
            std::uint16_t urgptr;
        };
    };
    
    inline void* getData()
    {
        return (void*)((unsigned long)this + TCPH_OFF + (doff << 2));
    }
    inline tcphdr* getTCPHeader()
    {
        return (tcphdr*)((unsigned long)this + TCPH_OFF);
    }
    static void* newInstance() //@ReflectDataMessageHeader
    {
        return snew DataMessageHeader();
    }
    inline int getTCPLen()
    {
        return tlen - TCPH_OFF - (doff << 2);
    }
    int modifyMSS(int tgt);
};

}

#endif
