#include "SimpleExecutable.hh"

using m3::SimpleExecutable;

const char *progname = "m3-mobilerelayapp";

int main(int argc, char **argv)
{
    SimpleExecutable *prog = SimpleExecutable::initInstance(progname, "MobileRelay");
    iferr (prog == 0)
    {
        logFatal("main: Initialization failed.");
    }
    
    prog->main(argc, argv);
}
