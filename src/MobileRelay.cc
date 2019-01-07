#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstdio>
#include <cstring>

#include "ConnectionClassifier.hh"
#include "ConnectionManager.hh"
#include "ContextManager.hh"
#include "HTTPDetector.hh"
#include "InterfaceManager.hh"
#include "InternalMessage.hh"
#include "Log.h"
#include "MobileRelay.hh"
#include "NetworkUtils.h"
#include "PacketBufferAllocator.hh"
#include "PacketInterceptor.hh"
#include "PolicyManager.hh"
#include "Represent.h"
#include "RRConnectionSetHandler.hh"
#include "DatabaseProvider.hh"

namespace m3
{
const char* MobileRelay::connmgrName = "ConnectionManager";
const char* MobileRelay::intfmgrName = "InterfaceManager";
const char* MobileRelay::clasfName = "ConnectionClassifier";
const char* MobileRelay::coxtmgrName = "ContextManager";
const char* MobileRelay::plcmgrName = "PolicyManager";
const char* MobileRelay::sourceName = "PacketInterceptor";

ConfigField MobileRelayConfig::myConfig[] =
{
    { 
        .name = "m3-sourceDevice", 
        .type = ConfigField::Type::STRING,
        .isCritical = true,
        .isValid = true
    },
    { 
        .name = "m3-sourceIP",
        .type = ConfigField::Type::STRING,
        .isCritical = true,
        .isValid = true
    },
    { 
        .name = "m3-myDevice",
        .type = ConfigField::Type::STRING,
        .isCritical = true,
        .isValid = true
    },
    { 
        .name = "m3-myIP",
        .type = ConfigField::Type::STRING,
        .isCritical = true,
        .isValid = true
    },
    { 
        .name = "m3-subnet",
        .type = ConfigField::Type::STRING,
        .isCritical = true,
        .isValid = true
    },
    {
        .name = "m3-flowAddrPairs",
        .type = ConfigField::Type::ARRAY,
        .isCritical = true,
        .isValid = true
    },
    {
        .name = "m3-fwmark",
        .type = ConfigField::Type::NUMBER,
        .isCritical = true,
        .isValid = true
    },
#ifdef BARE_FRAMEWORK
#else
    {
        .name = "m3-dbName",
        .type = ConfigField::Type::STRING,
        .isCritical = true,
        .isValid = true
    },
    {
        .name = "m3-gpsName",
        .type = ConfigField::Type::STRING,
        .isCritical = true,
        .isValid = true
    },
    {
        .name = "m3-dbInterfaceCount",
        .type = ConfigField::Type::NUMBER,
        .isCritical = true,
        .isValid = true
    },
#endif
    ConfigField::CLASS,
    ConfigField::EOL
};

ConfigField FlowAddressPairConfig::myConfig[] =
{
    { 
        .name = "device", 
        .type = ConfigField::Type::STRING,
        .isCritical = true,
        .isValid = true
    },
    { 
        .name = "src", 
        .type = ConfigField::Type::STRING,
        .isCritical = true,
        .isValid = true
    },
    { 
        .name = "dst", 
        .type = ConfigField::Type::STRING,
        .isCritical = true,
        .isValid = true 
    },
    { 
        .name = "srcp", 
        .type = ConfigField::Type::NUMBER,
        .isCritical = true,
        .isValid = true
    },
    { 
        .name = "dstp", 
        .type = ConfigField::Type::NUMBER,
        .isCritical = true,
        .isValid = true
    },
    ConfigField::CLASS,
    ConfigField::EOL
};

int MobileRelay::parseConfig(const char *config)
{
    int ret;
    
    iferr ((ret = myConfig.init(config)) != 0)
    {
        logStackTrace("MobileRelay::parseConfig", ret);
        return 1;
    }
    
    return 0;
}

int MobileRelay::init(void *arg)
{
    int ret;
    const char *config = (const char*)arg;
    PacketInterceptor::InitObject sourceInit;
    PacketBufferAllocator::InitObject pbInit;
    ConnectionManager::InitObject cmInit;
    char filter[64];
    ProgramPrototype::InitObject ppInit;

    ppInit.source = sourceName;
    ppInit.connmgr = connmgrName;
    ppInit.clasf = clasfName;
    ppInit.coxtmgr = coxtmgrName;
    ppInit.intfmgr = intfmgrName;
    ppInit.plcmgr = plcmgrName;
    iferr ((ret = ProgramPrototype::init(&ppInit)) != 0)
    {
        logStackTrace("MobileRelay::init", ret);
        return 1;
    }

    iferr ((ret = parseConfig(config)) != 0)
    {
        logStackTrace("MobileRelay::init", ret);
        return 2;
    }

    sourceInit.srcdev = myConfig.getString("m3-sourceDevice");
    iferr ((ret = inet_aton(myConfig.getString("m3-sourceIP"), 
        (in_addr*)&sourceInit.srcip)) == 0)
    {
        logError("MobileRelay::init: Invalid m3-sourceIP %s", 
            myConfig.getString("m3-sourceIP"));
        return 3;
    }

    sourceInit.fwddev = myConfig.getString("m3-myDevice");
    iferr ((ret = inet_aton(myConfig.getString("m3-myIP"), 
        (in_addr*)&sourceInit.fwdip)) == 0)
    {
        logError("MobileRelay::init: Invalid m3-myIP %s", 
            myConfig.getString("m3-myIP"));
        return 4;
    }

    sprintf(filter, "tcp and src net %s", myConfig.getString("m3-subnet"));
    sourceInit.filter = filter;
    sourceInit.scriptArgs = 0;
    sourceInit.fwmark = (unsigned short)myConfig.getInt("m3-fwmark");
    sourceInit.handletype = "PCAPInitializer";
    int snaplen = 262144;
    sourceInit.handlepriv = &snaplen;

    iferr ((ret = source->init(&sourceInit) != 0))
    {
        logStackTrace("MobileRelay::init", ret);
        return 5;
    }
    
    cmInit.unit.tv_sec = 10;
    cmInit.unit.tv_nsec = 0;
    cmInit.connMetaType = "ConnectionMetadataBase";
    for (int i = 0; i < Policy::Priority::count; ++i)
    {
        if ((cmInit.handlers[i] = snew RRConnectionSetHandler()) == 0)
        {
            logAllocError("MobileRelay::init");
            return 6;
        }
    }
    iferr ((ret = connmgr->init(&cmInit)) != 0)
    {
        logStackTrace("MobileRelay::init", ret);
        return 7;
    }
    
    iferr ((ret = plcmgr->init(coxtmgr, intfmgr)) != 0)
    {
        logStackTrace("MobileRelay::init", ret);
        return 8;
    }
    
    iferr ((ret = clasf->init(source, connmgr, plcmgr)) != 0)
    {
        logStackTrace("MobileRelay::init", ret);
        return 9;
    }

#ifdef BARE_FRAMEWORK
#else
    HTTPDetector *det;
    iferr ((det = snew HTTPDetector()) == 0)
    {
        logAllocError("MobileRelay::init");
        return 10;
    }
    clasf->addDetector(det);
#endif

    ConfigArray *addrarr = myConfig.getArray("m3-flowAddrPairs");
    iferr ((ret = intfmgr->init(connmgr, addrarr, clasf, source)) != 0)
    {
        logStackTrace("MobileRelay::init", ret);
        return 11;
    }

    int mssbs = intfmgr->getMinMSS() + DataMessageHeader::MSS_REDUCE_VAL * 2
        + sizeof(ethhdr);
    int mtubs = intfmgr->getMaxMTU() + DataMessageHeader::MSS_REDUCE_VAL;

    pbInit.count = 131072;
    pbInit.bs = 1 + (mssbs > mtubs ? mssbs : mtubs);
    logMessage("bs %d %d", pbInit.count, pbInit.bs);
    PacketBufferAllocator::getInstance()->init(&pbInit);
    ((PacketInterceptor*)source)->setMSS(intfmgr->getMinMSS());

    iferr ((ret = InternalMessage::initInternalMessage(connmgr, plcmgr)) != 0)
    {
        logStackTrace("MobileRelay::init", ret);
        return 12;
    }

#ifdef BARE_FRAMEWORK
#else
    timespec dbunit;
    const char *dbName = myConfig.getString("m3-dbName");
    dbunit.tv_nsec = 0;
    dbunit.tv_sec = 1;//1s
    dbprovider = snew DatabaseProvider();
    iferr (!dbprovider)
    {
        logAllocError("MobileRelay::init");
        return 14;
    }
    iferr((ret = dbprovider->init(dbName,
        myConfig.getInt("m3-dbInterfaceCount"), 
        myConfig.getString("m3-gpsName"), dbunit)) != 0)
    {
        logStackTrace("MobileRelay::init", ret);
        return 15;
    }
#endif

    return 0;
}

}
