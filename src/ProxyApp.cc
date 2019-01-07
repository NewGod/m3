#include "SimpleExecutable.hh"

using m3::SimpleExecutable;

const char *progname = "m3-proxyapp";
namespace m3
{
int proxyFlag = 1;
}

int main(int argc, char **argv)
{
    SimpleExecutable *prog = SimpleExecutable::initInstance(progname, "Proxy");
    iferr (prog == 0)
    {
        logFatal("main: Initialization failed.");
    }

    prog->main(argc, argv);
}
