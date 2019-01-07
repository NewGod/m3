#include "Log.h"
#include "PacketFilter.hh"

#include <netinet/tcp.h>
#include <netinet/udp.h>

namespace m3
{

static bool eval_bpf(struct bpf_node *root, iphdr *pak)
{
    switch(root->op)
    {
        case OP_AND:
        {
            auto ptr = root->child;
            while(ptr)
            {
                if(!eval_bpf(ptr, pak))
                    return false;
                ptr = ptr->next;
            }
            return true;
        }
        case OP_OR:
        {
            auto ptr = root->child;
            while(ptr)
            {
                if(eval_bpf(ptr, pak))
                    return true;
                ptr = ptr->next;
            }
            return false;
        }
        case OP_NOT:
            return !eval_bpf(root->child, pak);

        case OP_HOST:
        {
            unsigned int imask;
            unsigned char *mask = (unsigned char*)&imask;
            for(int i = 0; i < 4; i++)
                mask[i] = root->param.ip[i] ? 0xff : 0;
            bool match_src = (pak->saddr & imask) == root->param.val;
            bool match_dst = (pak->daddr & imask) == root->param.val;
            switch(root->param.dir)
            {
                case 0: return match_src || match_dst;
                case 1: return match_src;
                case 2: return match_dst;
                case 3: return match_src && match_dst;
                default: break;
            }
            break;
        }

        case OP_NET:
        {
            unsigned int imask;
            unsigned char *mask = (unsigned char*)&imask;
            for(int i = 0; i < 4; i++)
                mask[i] = root->param.ip[i] ? 0xff : 0;
            unsigned int smask = htonl((-1) << (32 - root->param.ip[4]));
            imask &= smask;
            bool match_src = (pak->saddr & imask) == (root->param.val & smask);
            bool match_dst = (pak->daddr & imask) == (root->param.val & smask);
            switch(root->param.dir)
            {
                case 0: return match_src || match_dst;
                case 1: return match_src;
                case 2: return match_dst;
                case 3: return match_src && match_dst;
                default: break;
            }
            break;
        }

        case OP_PORT:
        {
            if(root->param.proto != 0 && pak->protocol != root->param.proto)
                return false;
            if(pak->protocol != IPPROTO_TCP && pak->protocol != IPPROTO_UDP)
                return false;
            tcphdr *tcph = (tcphdr*)&pak[1];
            uint16_t sport = ntohs(tcph->th_sport),
                     dport = ntohs(tcph->th_dport);
            bool match_src =
                sport >= root->param.bound[0]
                && sport <= root->param.bound[1];
            bool match_dst =
                dport >= root->param.bound[0]
                && dport <= root->param.bound[1];
            switch(root->param.dir)
            {
                case 0: return match_src || match_dst;
                case 1: return match_src;
                case 2: return match_dst;
                case 3: return match_src && match_dst;
                default: break;
            }
            break;
        }

        case OP_PROTOCOL:
            return root->param.proto == IPPROTO_IP //ignore ip qualifier
                || pak->protocol == root->param.proto;

        default:
            return false;
    }
    return false;
}

PacketFilter::PacketFilter(const char* filter): root(nullptr)
{
    if (filter != 0)
    {
        root = bpf_compile(filter);
    }
}

PacketFilter::~PacketFilter()
{
    if(root) free_bpf(root);
}

int PacketFilter::init(const char* filter)
{
    logMessage("Filter %s", filter);
    if(root)
    {
        free_bpf(root);
        root = nullptr;
    }
    if(filter) root = bpf_compile(filter);
    return 0;
}

bool PacketFilter::drop(iphdr *pak)
{
    return root != 0 && !eval_bpf(root, pak);
}

}

#ifdef TEST

#include <cstdio>
const char *str = "beddae94d9eb962230771b6d080045000048327c40004006ead20a6404020afa0402818c091ecfc80beb00000000d00272101d9c0000020405b40402080a0f60fe6100000000010303071e0c0081b3e1a47f29845ec5";
int main()
{
    using namespace m3;
    PacketFilter *filt = new PacketFilter();
    puts("1");
    filt->init("tcp and tcp dst portrange 1-233 and ! src 1.2.3.4");
    puts("2");
    filt->init("tcp and src net 1.2.3/24");
    puts("3");
    filt->init("(tcp port 123 or tcp port 456) and (host 1.2.3.4 or host 5.6.7.8)");
    puts("4");
    filt->init("tcp and tcp dst portrange 50000-54095 and ! src 10.200.5.3");
    filt->init("tcp and src net 10.100.4");
    const char* ptr = str;
    char pak[100];
    while(*ptr)
    {
        sscanf(ptr, "%2hhx", pak + (ptr - str) / 2);
        ptr += 2;
    }
    for(int i = 0; i < 86; i++)
    {
        printf("%02hhx", pak[i]);
    }
    putchar('\n');
    bool res = filt->drop((iphdr*)&((ethhdr*)(void*)pak)[1]);
    printf("%d\n", (int)res);
}

#endif
