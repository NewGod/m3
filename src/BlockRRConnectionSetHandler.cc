#include "BlockRRConnectionSetHandler.hh"

namespace m3
{

int BlockRRConnectionSetHandler::issue(DataMessage *msg)
{
    int ret = ConnectionSetHandler::issue(msg);
    ifsucc (ret == 0)
    {
        cnt.release();
    }
    return ret;
}

int BlockRRConnectionSetHandler::next(Connection **dest)
{
    int ret;
    cnt.issue();
    iferr ((ret = RRConnectionSetHandler::next(dest)) != 0)
    {
        cnt.release();
    }

    return ret;
}

}