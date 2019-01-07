#ifndef __SIMPLEEXECUTABLE_HH__
#define __SIMPLEEXECUTABLE_HH__

#include <atomic>
#include <execinfo.h>

#include "Log.h"
#include "NetworkUtils.h"
#include "ProgramPrototype.hh"
#include "Reflect.hh"
#include "Represent.h"

namespace m3
{

// make this a singleton
class SimpleExecutable
{
protected:
    void *buf[1024];
    char btbuf[65536];
    char scriptbuf[131072];
    int argc;
    char **argv;
    std::atomic<bool> inHandler;
    static SimpleExecutable *instance;

    const char *progname;
    const char *progtype;
    const char *logPath;
    const char *configPath;
    ProgramPrototype *prog;
    
    void parseArguments(int argc, char **argv);
    static void sigHandler(int sig, siginfo_t *info, void *ptr);
    static int printStackTrace(void *trace, int n);
    static int printStackTraceFallback(void *trace, int n);
    virtual void cleanup();
    SimpleExecutable(const char *progname, const char *progtype): 
        inHandler(false), progname(progname), progtype(progtype)
    {
        instance = this;
    }
public:
    static inline SimpleExecutable *initInstance(
        const char *progname, const char *progtype)
    {
        iferr (instance == 0)
        {
            instance = snew SimpleExecutable(progname, progtype);
        }
        return instance;
    }
    static inline SimpleExecutable *getInstance()
    {
        return initInstance(0, 0);
    }
    int main(int argc, char **argv);
};

}

#endif
