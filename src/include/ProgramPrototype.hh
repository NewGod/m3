#ifndef __PROGRAMPROTOTYPE_HH__
#define __PROGRAMPROTOTYPE_HH__

namespace m3
{

class ConnectionManager;
class InterfaceManager;
class ConnectionClassifier;
class ContextManager;
class PolicyManager;
class PacketSource;

class ProgramPrototype
{
public:
    struct InitObject
    {
        const char *connmgr;
        const char *intfmgr;
        const char *clasf;
        const char *coxtmgr;
        const char *plcmgr;
        const char *source;
    };
    ProgramPrototype();
    virtual ~ProgramPrototype();

    virtual int init(void *arg);
    void wait();
    int setLocale();
protected:
    ConnectionManager *connmgr;
    InterfaceManager *intfmgr;
    ConnectionClassifier *clasf;
    ContextManager *coxtmgr;
    PolicyManager *plcmgr;
    PacketSource *source;
};

}

#endif
