#include <arpa/inet.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <cstdlib>
#include <cstring>

#include "DeviceMetadata.hh"
#include "NetworkUtils.h"
#include "Log.h"

namespace m3
{

int DeviceMetadata::setMetadata(const char *name)
{
    ifreq hwaddr, ipaddr;
    int ret;

    if ((ret = getDeviceAddress(name, &hwaddr, &ipaddr)) != 0)
    {
        logError("DeviceMetadata::setMetadata: Can't get device"
            " information for device %s(%d).", name, ret);
        return 1;
    }

    strncpy(this->name, name, IFNAMSIZ);
    memcpy(this->mac, &hwaddr.ifr_hwaddr.sa_data, ETH_ALEN);
    memcpy(&this->ip, 
        &((sockaddr_in*)&ipaddr.ifr_addr)->sin_addr, sizeof(in_addr));

    logDebug("name %s, ether %s, ip %s", this->name, 
        ether_ntoa((const ether_addr*)this->mac), 
        inet_ntoa(*(in_addr*)&this->ip));

    return 0;
}


}