#ifndef __LOG_H__
#define __LOG_H__

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef  __cplusplus  
#include <atomic>
#include <thread>
typedef std::atomic<int> lock_t;
#define memory_order_relaxed std::memory_order_relaxed
extern "C" {
#else
#include <stdatomic.h>
typedef _Atomic int lock_t;
#endif

// set them use gcc -D option.
#ifndef PROGNAME
#define PROGNAME "Undefined"
#endif
#ifndef VERSION
#define VERSION "Undefined"
#endif

extern int verbose;
extern int logFD;

#define LOGBUF_LEVEL 12
#define LOGBUF_LEN 512
extern lock_t logNum;
extern char logBuf[1 << LOGBUF_LEVEL][LOGBUF_LEN << 1];

extern ssize_t (*logWrite)(int, const void*, size_t);

void setVerbose(int level);

void initLog();
void resetLogFile();
double getTimestamp();
void printInitLog();
char *strerrorV(int num, char *buf);

static inline unsigned long getmytid()
{
    // OK for c++11, maybe BUGGY in later c++ standards.
    return pthread_self();
}

static inline char* getBuf()
{
    return logBuf[atomic_fetch_add_explicit(&logNum, 1, memory_order_relaxed) 
        & ((1 << LOGBUF_LEVEL) - 1)];
}

// A NEW doLog macro without locks
// note that to improve efficiency, we are not blocking signals anymore.
// According write(2), write() is not atomic in kernel < 3.14, thus race
// can happen in such condition.
#define vanillaDoLog(buf, prefix, ...)                              \
    {                                                               \
        sprintf(buf + LOGBUF_LEN, __VA_ARGS__);                     \
        sprintf(buf, "%s{%5lx}(%14.6lf): %s\n", prefix,             \
            getmytid(), getTimestamp(), buf + LOGBUF_LEN);          \
        logWrite(logFD, buf, strlen(buf));                          \
    }

#define doLog(prefix, ...)                                          \
    {                                                               \
        char *__dolog = getBuf();                                   \
        vanillaDoLog(__dolog, prefix, __VA_ARGS__);                 \
    }

#ifdef ENABLE_DEBUG_LOG
#define logDebug(...)                                               \
    do                                                              \
    {                                                               \
        doLog("[  Debug  ]", __VA_ARGS__);                          \
    } while (0)
#else
#define logDebug(...) ;
#endif

#define logVerboseL(level, ...)                                     \
    do                                                              \
    {                                                               \
        if (verbose >= level)                                       \
        {                                                           \
            doLog("[ Verbose ]", __VA_ARGS__);                      \
        }                                                           \
    } while (0)

#define logVerbose(...) logVerboseL(1, __VA_ARGS__)

#define logMessage(...)                                             \
    do                                                              \
    {                                                               \
        doLog("[ Message ]", __VA_ARGS__);                          \
    } while (0)

#define logWarning(...)                                             \
    do                                                              \
    {                                                               \
        doLog("[ Warning ]", __VA_ARGS__);                          \
    } while (0)

#define logError(...)                                               \
    do                                                              \
    {                                                               \
        doLog("[  Error  ]", __VA_ARGS__);                          \
    } while (0)

#define logStackTrace(name, res)\
    logError("%s: Called from here(%ld).", name, (long)res)
    
#define logStackTraceEnd()\
    logError("Stack trace ends here.");

#define logAllocError(name)\
    logError("%s: Allocation failed. Cannot create object.", name)

#define logFatal(...)                                               \
    do                                                              \
    {                                                               \
        doLog("[  FATAL  ]", __VA_ARGS__);                          \
        exit(1);                                                    \
    } while (0)

extern const char *usage;
static inline void printUsageAndExit(char **argv)
{
    fprintf(stderr, "Usage: %s [options]\n%s\n", argv[0], usage);
    exit(0);
}

static inline void printVersionAndExit(const char *name)
{
    fprintf(stderr, "%s %s\n", name, VERSION);
    exit(0);
}

static inline void redirectLogTo(const char *path)
{
    int newFD;
    char errbuf[256];
    
    // we don't mean to close the stdout/err files here.
    if (logFD != STDOUT_FILENO && logFD != STDERR_FILENO)
    {
        close(logFD);
    }
    if ((newFD = open(path, O_APPEND | O_WRONLY | O_CREAT, 0644)) == -1)
    {
        logFatal("Can't open log file %s(%s).", path, strerrorV(errno, errbuf));
    }
    else
    {
        logFD = newFD;
    }
}

#ifdef  __cplusplus  
} 
#endif  

#endif
