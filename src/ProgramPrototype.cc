#include <cstdio>
#include <cstring>
#include <locale>

#include <locale.h>
#include <unistd.h>

#include "ConnectionClassifier.hh"
#include "ConnectionManager.hh"
#include "ContextManager.hh"
#include "InterfaceManager.hh"
#include "Log.h"
#include "PacketSource.hh"
#include "PolicyManager.hh"
#include "ProgramPrototype.hh"
#include "Reflect.hh"
#include "Represent.h"

namespace m3
{

ProgramPrototype::ProgramPrototype():
    connmgr(0), intfmgr(0), clasf(0), coxtmgr(0), plcmgr(0),
    source(0) {}

ProgramPrototype::~ProgramPrototype()
{
    clearptr(connmgr);
    clearptr(intfmgr);
    clearptr(clasf);
    clearptr(coxtmgr);
    clearptr(plcmgr);
    clearptr(source);
}

int ProgramPrototype::setLocale()
{
    iferr (setlocale(LC_ALL, "C") == 0)
    {
        logError("ProgramPrototype::setLocale: Cannot modify C locale.");
        return 1;
    }

    std::locale::global(std::locale());

    return 0;
}

void ProgramPrototype::wait()
{
    while (1)
    {
        sleep(10);
    }
}

int ProgramPrototype::init(void *arg)
{
    int ret;

    InitObject *iarg = (InitObject*)arg;
    
    iferr ((ret = setLocale()) != 0)
    {
        logStackTrace("ProgramPrototype::createObject", ret);
        return 1;
    }

    iferr ((this->clasf = (ConnectionClassifier*)Reflect::create(
        iarg->clasf)) == 0)
    {
        logStackTrace("ProgramPrototype::createObject",
            this->clasf);
        return 2;
    }
    
    iferr ((this->intfmgr = (InterfaceManager*)Reflect::create(
        iarg->intfmgr)) == 0)
    {
        logStackTrace("ProgramPrototype::createObject",
            this->intfmgr);
        return 3;
    }
    
    this->coxtmgr = ContextManager::getInstance();
    
    iferr ((this->plcmgr = (PolicyManager*)Reflect::create(
        iarg->plcmgr)) == 0)
    {
        logStackTrace("ProgramPrototype::createObject",
            this->plcmgr);
        return 5;
    }
    
    iferr ((this->source = (PacketSource*)Reflect::create(
        iarg->source)) == 0)
    {
        logStackTrace("ProgramPrototype::createObject",
            this->source);
        return 6;
    }
    
    iferr ((this->connmgr = (ConnectionManager*)Reflect::create(
        iarg->connmgr)) == 0)
    {
        logStackTrace("ProgramPrototype::createObject",
            this->connmgr);
        return 7;
    }
    
    return 0;
}

}