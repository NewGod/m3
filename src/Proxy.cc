#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstdio>
#include <cstring>

#include "ConnectionClassifier.hh"
#include "ConnectionManager.hh"
#include "ContextManager.hh"
#include "HTTPDetector.hh"
#include "InternalMessage.hh"
#include "Log.h"
#include "Proxy.hh"
#include "NetworkUtils.h"
#include "PacketBufferAllocator.hh"
#include "PolicyManager.hh"
#include "Represent.h"
#include "RRConnectionSetHandler.hh"
#include "ServerInterfaceManager.hh"
#include "ServerPacketSource.hh"
#include "DatabaseProvider.hh"

namespace m3
{
const char* Proxy::connmgrName = "ConnectionManager";
const char* Proxy::intfmgrName = "ServerInterfaceManager";
const char* Proxy::clasfName = "ConnectionClassifier";
const char* Proxy::coxtmgrName = "ContextManager";
const char* Proxy::plcmgrName = "PolicyManager";
const char* Proxy::sourceName = "ServerPacketSource";

ConfigField ProxyConfig::myConfig[] =
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
        .name = "m3-ListenAddress",
        .type = ConfigField::Type::ARRAY,
        .isCritical = true,
        .isValid = true
    },
    {
        .name = "m3-portBase",
        .type = ConfigField::Type::NUMBER,
        .isCritical = true,
        .isValid = true
    },
    {
        .name = "m3-portLen",
        .type = ConfigField::Type::NUMBER,
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
        .name = "m3-dbInterfaceCount",
        .type = ConfigField::Type::NUMBER,
        .isCritical = true,
        .isValid = true
    },
#endif
    ConfigField::CLASS,
    ConfigField::EOL
};

ConfigField ListenAddressConfig::myConfig[] =
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
        .name = "srcp", 
        .type = ConfigField::Type::NUMBER,
        .isCritical = true,
        .isValid = true
    },
    { 
        .name = "count", 
        .type = ConfigField::Type::NUMBER,
        .isCritical = false,
        .isValid = true
    },
    ConfigField::CLASS,
    ConfigField::EOL
};

int Proxy::parseConfig(const char *config)
{
    int ret;
    
    iferr ((ret = myConfig.init(config)) != 0)
    {
        logStackTrace("Proxy::parseConfig", ret);
        return 1;
    }
    
    return 0;
}

int Proxy::init(void *arg)
{
    int ret;
    const char *config = (const char*)arg;
    ServerPacketSource::InitObject sourceInit;
    PacketBufferAllocator::InitObject pbInit;
    ConnectionManager::InitObject cmInit;
    ProgramPrototype::InitObject ppInit;

    ppInit.source = sourceName;
    ppInit.connmgr = connmgrName;
    ppInit.clasf = clasfName;
    ppInit.coxtmgr = coxtmgrName;
    ppInit.intfmgr = intfmgrName;
    ppInit.plcmgr = plcmgrName;
    iferr ((ret = ProgramPrototype::init(&ppInit)) != 0)
    {
        logStackTrace("Proxy::init", ret);
        return 1;
    }

    iferr ((ret = parseConfig(config)) != 0)
    {
        logStackTrace("Proxy::init", ret);
        return 2;
    }

    sourceInit.initPI.srcdev = myConfig.getString("m3-sourceDevice");
    iferr ((ret = inet_aton(myConfig.getString("m3-sourceIP"), 
        (in_addr*)&sourceInit.initPI.srcip)) == 0)
    {
        logError("Proxy::init: Invalid m3-sourceIP %s", 
            myConfig.getString("m3-sourceIP"));
        return 3;
    }

    sourceInit.initPI.fwddev = myConfig.getString("m3-myDevice");
    iferr ((ret = inet_aton(myConfig.getString("m3-myIP"), 
        (in_addr*)&sourceInit.initPI.fwdip)) == 0)
    {
        logError("Proxy::init: Invalid m3-myIP %s", 
            myConfig.getString("m3-myIP"));
        return 4;
    }

    sourceInit.portBase = (unsigned short)myConfig.getInt("m3-portBase");
    sourceInit.portLen = (unsigned short)myConfig.getInt("m3-portLen");
    sourceInit.initPI.fwmark = (unsigned short)myConfig.getInt("m3-fwmark");
    sourceInit.initPI.handletype = "RawSocketInitializer";
    int snaplen = 262144;
    sourceInit.initPI.handlepriv = &snaplen;

    iferr ((ret = source->init(&sourceInit)) != 0)
    {
        logStackTrace("Proxy::init", ret);
        return 4;
    }
    
    cmInit.unit.tv_sec = 10;
    cmInit.unit.tv_nsec = 0;
    cmInit.connMetaType = "ConnectionMetadataBase";
    for (int i = 0; i < Policy::Priority::count; ++i)
    {
        if ((cmInit.handlers[i] = snew RRConnectionSetHandler()) == 0)
        {
            logAllocError("Proxy::init");
            return 5;
        }
    }
    iferr ((ret = connmgr->init(&cmInit)) != 0)
    {
        logStackTrace("Proxy::init", ret);
        return 6;
    }
    
    iferr ((ret = plcmgr->init(coxtmgr, intfmgr)) != 0)
    {
        logStackTrace("Proxy::init", ret);
        return 7;
    }
    
    iferr ((ret = clasf->init(source, connmgr, plcmgr)) != 0)
    {
        logStackTrace("Proxy::init", ret);
        return 8;
    }

#ifdef BARE_FRAMEWORK
#else
    HTTPDetector *det;
    iferr ((det = snew HTTPDetector()) == 0)
    {
        logAllocError("Proxy::init");
        return 9;
    }
    clasf->addDetector(det);
#endif

    ConfigArray *addrarr = myConfig.getArray("m3-ListenAddress");
    iferr ((ret = intfmgr->init(connmgr, addrarr,
        clasf, source)) != 0)
    {
        logStackTrace("Proxy::init", ret);
        return 10;
    }

    int mssbs = intfmgr->getMinMSS() + DataMessageHeader::MSS_REDUCE_VAL * 2
        + sizeof(ethhdr);
    int mtubs = intfmgr->getMaxMTU() + DataMessageHeader::MSS_REDUCE_VAL;

    pbInit.count = 131072;
    pbInit.bs = 1 + (mssbs > mtubs ? mssbs : mtubs);
    PacketBufferAllocator::getInstance()->init(&pbInit);
    ((PacketInterceptor*)source)->setMSS(intfmgr->getMinMSS());

    iferr ((ret = InternalMessage::initInternalMessage(connmgr, plcmgr)) != 0)
    {
        logStackTrace("Proxy::init", ret);
        return 11;
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
        logAllocError("Proxy::init");
        return 12;
    }
    iferr((ret = dbprovider->init(dbName, 
        myConfig.getInt("m3-dbInterfaceCount"), nullptr, dbunit)) != 0)
    {
        logStackTrace("Proxy::init", ret);
        return 13;
    }
#endif

    return 0;
}

}