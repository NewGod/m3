#ifndef __TYPEDETECTOR_HH__
#define __TYPEDETECTOR_HH__

#include "ConnectionType.hh"

namespace m3
{

class DataMessage;

// Different `TypeDetector`s are expected to be detecting __completely__
// different types.
class TypeDetector
{
public:
    enum Type { HTTPDetector = 0 };

    virtual ~TypeDetector() {}
    virtual Type getType() = 0;
    virtual ConnectionType::Type detectFrom(DataMessage *msg) = 0;
};

}

#endif
