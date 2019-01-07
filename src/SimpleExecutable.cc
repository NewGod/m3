#include "SimpleExecutable.hh"

const char *usage = 
    "  -f [path]:\n"
    "    Specify config file path, use \"-\" to read config from stdin.\n"
    "    (default value is \"-\")\n"
    "  -h:\n"
    "    Print this message and exit.\n"
    "  -l [path]:\n"
    "    Specify the file to save the log.\n"
    "  -v:\n"
    "    Print version information and exit.\n"
    "  -V[level]:\n"
    "    Print verbose log. Use -V2 for even more verbose log.";

namespace m3
{

SimpleExecutable *SimpleExecutable::instance = 0;

void SimpleExecutable::parseArguments(int argc, char **argv)
{
    char c;
    int verbose = 0;
    optind = 0;
    while ((c = getopt(argc, argv, "f:hl:vV::")) != EOF)
    {
        switch (c)
        {
        case 'f':
            configPath = optarg;
            break;
        case 'h':
            printUsageAndExit(argv);
            break;
        case 'l':
            logPath = optarg;
            break;
        case 'v':
            printVersionAndExit(progname);
            break;
        case 'V': 
            if (optarg)
            {
                verbose = atoi(optarg);
            }
            else
            {
                verbose = 1;
            }
            break;
        default:
            logWarning("Unexpected command-line option %c!", (char)optopt);
            printUsageAndExit(argv);
        }
    }

    setVerbose(verbose);
    if (logPath != 0)
    {
        redirectLogTo(logPath);
    }
    if (configPath == 0)
    {
        configPath = "-";
    }
    SimpleExecutable::argc = argc;
    SimpleExecutable::argv = argv;
}

int SimpleExecutable::printStackTrace(void *trace, int n)
{
    FILE *fp;
    SimpleExecutable *me = getInstance();
    unsigned long base;
    sprintf(me->scriptbuf, "cat /proc/%d/maps | grep r-xp | grep %s |"
        "awk -F '-' '{print $1}'", getpid(), me->argv[0]);
    iferr ((fp = popen(me->scriptbuf, "r")) == 0 ||
        fscanf(fp, "%lx", &base) < 1)
    {
        logError("SimpleExecutable::printStackTrace: Can't read memory mapping"
            " information.");
        return 1;
    }
    pclose(fp);

    char *pos = me->btbuf;
    for (int i = 0; i < n; ++i)
    {
        sprintf(pos, "%lx ", (unsigned long)me->buf[i] - base);
        pos += strlen(pos);
    }
    sprintf(me->scriptbuf, "addr2line %s -a -p -f -C -e %s", me->btbuf, me->argv[0]);
    logVerbose("%s", me->scriptbuf);
    if (system(me->scriptbuf) < 0)
    {
        logError("Failed to print stack trace using addr2line.");
        return 2;
    }
    return 0;
}

int SimpleExecutable::printStackTraceFallback(void *trace, int n)
{
    SimpleExecutable *me = getInstance();
    char **strings = backtrace_symbols(me->buf, n);
    for (int i = 0; i < n; i++)
    {
        logError("%s", strings[i]);
    }
    return 0;
}

void SimpleExecutable::sigHandler(int sig, siginfo_t *info, void *ptr)
{
    SimpleExecutable *me = getInstance();
    int n;
    bool desired = false; 
    if (!me->inHandler.compare_exchange_strong(desired, true))
    {
        return;
    }

    logError("SimpleExecutable::sigHandler: Signal %d received", sig);

    n = backtrace(me->buf, 1024);

    logError("Stack trace:");
    if (printStackTrace(me->buf, n) != 0)
    {
        logError("Fallback...");
        printStackTraceFallback(me->buf, n);
    }
    logError("Stack trace ends here.");

    instance->cleanup();
    logFatal("SimpleExecutable::sigHandler: Signaled, exit now.");
}

void SimpleExecutable::cleanup()
{
    if (prog)
    {
        delete prog;
    }
}

int SimpleExecutable::main(int argc, char **argv)
{
    initLog();  

    parseArguments(argc, argv);
    printInitLog();

    logMessage("SimpleExecutable::main: Thread identify");
    logMessage("SimpleExecutable::main: Running as %s, using program %s", 
        progname, progtype);

    signalNoRestart(SIGINT, sigHandler);
    signalNoRestart(SIGSEGV, sigHandler);
    signalNoRestart(SIGPIPE, sigHandler);
    signalNoRestart(SIGTERM, sigHandler);
    signalNoRestart(SIGABRT, sigHandler);
    signalNoRestart(SIGFPE, sigHandler);
    iferr ((prog = (ProgramPrototype*)Reflect::create(progtype)) == 0)
    {
        logAllocError("SimpleExecutable");
        logFatal("Cannot create program.");
    }

    int ret;
    if ((ret = prog->init((void*)configPath)) != 0)
    {
        logStackTrace("SimpleExecutable::main", ret);
        logStackTraceEnd();
        delete prog;
        logFatal("Program initialization failed.");
    }
    logMessage("SimpleExecutable::main: Initialize complete.");

    prog->wait();
    delete prog;

    return 0;
}

}
