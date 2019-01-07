#include <atomic>

#include "ContextManager.hh"
#include "DataMessageHeader.hh"
#include "InternalMessage.hh"
#include "InternalMessageParser.hh"
#include "Log.h"
#include "Represent.h"

namespace m3
{

int InternalMessageParser::initContext()
{
    static std::atomic<bool> fe(true);

    iferr (fe.exchange(false))
    {
        InternalMessageContext *coxt = snew InternalMessageContext();
        iferr (coxt == 0)
        {
            logAllocError("InternalMessageParser::initContext");
            return 1;
        }

        ContextManager::getInstance()->addContext(coxt);
    }

    return 0;
}

#ifdef INTERNALMESSAGE_DEBUG
class DebugWorker : public std::thread
{
protected:
    static DebugWorker *dw;
    int n;
    static void main(DebugWorker *dw)
    {
        sleep(5);
        while (1)
        {
            int *buf;
            sleep(1);

            buf = (int*)InternalMessage::getMessageBuf();
            if (buf == 0)
            {
                logMessage("???");
                continue;
            }
            *buf = dw->n++;
            logMessage("Send Message %d", *buf);
            InternalMessage::sendInternalMessage(new InternalMessage(
                buf, 4));
        }
    }
public:
    DebugWorker(): std::thread(main, this), n(0) {}
};

DebugWorker *DebugWorker::dw = new DebugWorker();
#endif

InternalMessageParser::InternalMessageParser()
{
    iferr (initContext() != 0)
    {
        throw 0;
    }
    iferr (ContextManager::getInstance()->getContext(
        Context::Type::INTERNAL, (Context**)&coxt) != 0)
    {
        throw 1;
    }
}

int InternalMessageParser::parse(DataMessage *msg)
{
    if (!isInternalMessage(msg))
    {
        return 1;
    }
    else
    {
#ifdef INTERNALMESSAGE_DEBUG
        int *data = (int*)((DataMessageMetaHeader*)msg->getHeader() + 1);
        logMessage("Message %lx", (unsigned long)data);
        logMessage("Message %d", *data);
#endif
        // we assume we are the only writer of `coxt`
        iferr (coxt->tryAdvance((InternalMessage**)&msg) != 0)
        {
            InternalMessage *tmp = 0;
            coxt->tryPurchase(&tmp);
            delete tmp;
            logWarning("InternalMessageParser::parse: "
                "Buffer full, drop message %lx", (unsigned long)tmp);
            coxt->tryAdvance((InternalMessage**)&msg);
        }
        return 0;
    }
}

}