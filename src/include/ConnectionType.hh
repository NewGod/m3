#ifndef __CONNECTIONTYPE_HH__
#define __CONNECTIONTYPE_HH__

#include "AutoDestroyable.hh"

#include <atomic>

namespace m3
{

class TypeDetector;

// How to add a new ConnectionType:
// 1. add an item to Types
// 2. implement a class inherits ConnectionType, implement getID() to let
//    it return the item you added.
class ConnectionType : public AutoDestroyable
{
public:
    enum Type { NOTYPE = 0, HTTP_GENERIC, VIDEO, WEB, count };

    ConnectionType(): AutoDestroyable() {}
    
    virtual TypeDetector *getDetector() = 0;
    virtual ~ConnectionType() {}
    virtual Type getType() = 0;
};

}

#endif
