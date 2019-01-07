#ifndef __PROXY_HH__
#define __PROXY_HH__

#include "Config.hh"
#include "ProgramPrototype.hh"

namespace m3
{

struct ProxyConfig : public ConfigObject 
{
    ProxyConfig() {}
    static void* newInstance() //@ReflectProxyConfig
    {
        return snew ProxyConfig();
    }
protected:
    static ConfigField myConfig[];
    ConfigField* getFields() override
    {
        return myConfig;
    }
};

struct ListenAddressConfig : public ConfigObject 
{
    ListenAddressConfig() {}
    static void* newInstance() //@ReflectListenAddressConfig
    {
        return snew ListenAddressConfig();
    }
protected:
    static ConfigField myConfig[];
    ConfigField* getFields() override
    {
        return myConfig;
    }
};

class Proxy : public ProgramPrototype
{
public:
    Proxy() {}
    int init(void *arg) override;

    static void* newInstance() //@ReflectProxy
    {
        return snew Proxy();
    }
protected:
    static const char* connmgrName;
    static const char* intfmgrName;
    static const char* clasfName;
    static const char* coxtmgrName;
    static const char* plcmgrName;
    static const char* sourceName;
    
    int parseConfig(const char *config);
    int initInterfaces(ConfigArray *pairs);
    ProxyConfig myConfig;
};

}

#endif
