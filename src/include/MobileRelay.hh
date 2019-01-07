#ifndef __MOBILERELAY_HH__
#define __MOBILERELAY_HH__

#include <vector>

#include "Config.hh"
#include "ProgramPrototype.hh"

namespace m3
{

struct MobileRelayConfig : public ConfigObject 
{
    MobileRelayConfig() {}
    static void* newInstance() //@ReflectMobileRelayConfig
    {
        return snew MobileRelayConfig();
    }
protected:
    static ConfigField myConfig[];
    ConfigField* getFields() override
    {
        return myConfig;
    }
};

struct FlowAddressPairConfig : public ConfigObject
{
    FlowAddressPairConfig() {}
    static void* newInstance() //@ReflectFlowAddressPairConfig
    {
        return snew FlowAddressPairConfig();
    }
protected:
    static ConfigField myConfig[];
    ConfigField* getFields() override
    {
        return myConfig;
    }
};

class MobileRelay : public ProgramPrototype
{
public:
    MobileRelay() {}
    int init(void *arg) override;

    static void* newInstance() //@ReflectMobileRelay
    {
        return snew MobileRelay();
    }
protected:
    static const char* connmgrName;
    static const char* intfmgrName;
    static const char* clasfName;
    static const char* coxtmgrName;
    static const char* plcmgrName;
    static const char* sourceName;
    
    int parseConfig(const char *config);
    MobileRelayConfig myConfig;
};

}

#endif
