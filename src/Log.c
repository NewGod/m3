#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include "Log.h"

int verbose = 0;
lock_t logLock;
int logFD = STDERR_FILENO;
static struct timespec tst;
lock_t logNum;
char logBuf[1 << LOGBUF_LEVEL][LOGBUF_LEN << 1];
ssize_t (*logWrite)(int, const void*, size_t) = write;

char *strerrorV(int num, char *buf)
{
    sprintf(buf, "%d ", num);
    strerror_r(num, buf + strlen(buf), 128);
    return buf;
}

void setVerbose(int level)
{
    verbose = level;
}

void printInitLog()
{
    time_t rawtime = tst.tv_sec;
    struct tm timeinfo;
    char timeStr[80];

    localtime_r(&rawtime, &timeinfo);
    strftime(timeStr, 80, "%z %Y-%m-%d %H:%M:%S", &timeinfo);

    logMessage("%s log \"%s\"", PROGNAME, VERSION);
    logMessage("Timestamp initialized at %s(%lf)", timeStr,
               tst.tv_sec + tst.tv_nsec / (double)1000000000.0);
}

void initLog()
{
    clock_gettime(CLOCK_REALTIME_COARSE, &tst);
}

double getTimestamp()
{
    struct timespec ted;
    clock_gettime(CLOCK_REALTIME_COARSE, &ted);
    return (ted.tv_sec - tst.tv_sec) + 
        (ted.tv_nsec - tst.tv_nsec) / 1000000000.0;
}

void resetLogFile()
{
    if (logFD != STDERR_FILENO && logFD != STDOUT_FILENO)
    {
        close(logFD);
    }
    logFD = STDERR_FILENO;
}
