#ifndef __BPF_H__
#define __BPF_H__

#include <netinet/ip.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef unsigned char uchar;
typedef unsigned int uint;

enum bpf_op_type
{
    OP_AND,
    OP_OR,
    OP_NOT,
    OP_HOST,
    OP_NET,
    OP_PORT,
    OP_PROTOCOL
};

struct bpf_node
{
    struct bpf_node *child;
    struct bpf_node *next;
    struct bpf_node *parent;
    enum bpf_op_type op;
    struct
    {
        union
        {
            uchar ip[5];
            uchar mac[6];
            uint bound[2];
            uint val;
        };
        uint proto;
        uint dir;
    } param;
};

struct bpf_node* bpf_compile(const char* filter);
void free_bpf(struct bpf_node* root);

#ifdef __cplusplus
}
#endif

#endif
