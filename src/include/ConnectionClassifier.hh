#ifndef __CONNECTIONCLASSIFIER_HH__
#define __CONNECTIONCLASSIFIER_HH__

#include <cstring>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "IPPacket.hh"
#include "PacketSource.hh"
#include "PolicyManager.hh"
#include "StringUtils.h"

namespace m3
{

class DataMessage;
class TypeDetector;

class ConnectionClassifier
{
public:
    static const int MAX_DETECTOR = 8;
    static void* newInstance() //@ReflectConnectionClassifier
    {
        return snew ConnectionClassifier();
    }
    int init(PacketSource *source, ConnectionManager *connmgr, 
        PolicyManager *plcmgr);
    void addDetector(TypeDetector *det);
    void removeDetector(TypeDetector *det);
    void detect(DataMessage *dmsg);

    class ClassifyWorker : public std::thread
    {
    protected:
        static void tmain(
            ConnectionClassifier *clasf, 
            PacketSource *source, 
            ConnectionManager *connmgr, 
            PolicyManager *plcmgr);
    public:
        inline ClassifyWorker(
            ConnectionClassifier *clasf,
            PacketSource *source, 
            ConnectionManager *connmgr, 
            PolicyManager *plcmgr) :
            std::thread(tmain, clasf, source, connmgr, plcmgr) {}
    };
protected:
    ClassifyWorker *cw;

    PacketSource *source;
    ConnectionManager *connmgr;
    PolicyManager *plcmgr;
    TypeDetector* detectors[MAX_DETECTOR];
};

}

#endif
