#ifndef __DEVICEMETADATA_HH__
#define __DEVICEMETADATA_HH__

#include <sys/socket.h>
#include <sys/types.h>
#include <linux/if.h>
#include <netinet/ether.h>

namespace m3
{

struct DeviceMetadata
{
    char name[IFNAMSIZ + 1];
    unsigned ip;
    char mac[ETH_ALEN];

    int setMetadata(const char *name);
};

}

#endif
